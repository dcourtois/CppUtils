#ifndef TASK_MANAGER_H
#define TASK_MANAGER_H


#include <atomic>
#include <cassert>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>


//!
//! The task manager allows to easily create a pool thread and send jobs to
//! it. Each job will be executed on an independent thread, in a FIFO fashion.
//! It also provides a cheap thread local storage emulation capability.
//!
class TaskManager
{

public:

	//!
	//! Defines a simple task
	//!
	typedef std::function< void (void *) > Task;

	//!
	//! Constructor.
	//!
	//! @param threadCount
	//!		Optional number of thread used by the task manager. If omitted, the number
	//!		of thread created will be the one supported by the platform.
	//!
	TaskManager(int threadCount = -1)
		: m_Stop(false)
		, m_QueuedTaskCount(0)
		, m_RunningTaskCount(0)
	{
		// init the threads
		this->SetThreadCount(threadCount, false);
	}

	//!
	//! Destructor. Cleanup the threads. This will stall the current
	//! thread until all jobs are done processing and all threads have
	//! been correctly stopped and cleaned up.
	//!
	~TaskManager(void)
	{
		m_Stop = true;
		m_ConditionVariable.notify_all();
		this->JoinThreads();
	}

	//!
	//! Checks if there is a task queued.
	//!
	inline bool HasTask(void)
	{
		return m_QueuedTaskCount > 0;
	}

	//!
	//! Set the thread local storage for the given thread.
	//!
	//! @param threadIndex
	//!		The 0 based index of the thread. See GetNumThreads.
	//!
	//! @param data
	//!		Pointer to the storage. The TaskManager will *NOT* own
	//!		this data, you are responsible for cleaning it.
	//!
	inline void SetThreadLocalStorage(int threadIndex, void * data)
	{
		assert(threadIndex >= 0 && threadIndex < static_cast< int >(m_ThreadLocalStorage.size()));
		m_ThreadLocalStorage[threadIndex] = data;
	}

	//!
	//! Push a new task. The task can be anything that is castable to
	//! a `std::function< void (void *) >`. Implicit cast will work with
	//! lambdas and static methods / functions, and you can bind a method
	//! of an instance using `std::bind` explicitely.
	//!
	//! The `void *` parameter passed to the task is a pointer to some
	//! user data (use SetThreadLocalStorage to set this) that is shared
	//! per-thread. If you do not need per-thread data, just use the other
	//! PushTask(std::function< void (void) > &&) version.
	//!
	inline void PushTask(Task && task)
	{
		// do nothing if we're stopping
		if (m_Stop == true)
		{
			return;
		}

		// check if we have some threads
		if (m_Threads.empty() == true)
		{
			// no jobs, just execute the task
			++m_RunningTaskCount;
			task(m_ThreadLocalStorage.front());
			--m_RunningTaskCount;
		}
		else
		{
			// push the job
			{
				std::lock_guard< std::mutex > lock(m_QueueMutex);
				m_Queue.emplace(task);
				++m_QueuedTaskCount;
			}

			// and notify one thread that we have a job to do
			m_ConditionVariable.notify_one();
		}
	}

	//!
	//! Push a new task where the user do not need thread's local storage.
	//! See PushTask(Task && task)
	//!
	inline void PushTask(std::function< void (void) > && task)
	{
		this->PushTask([=] (void *) { task(); });
	}

	//!
	//! Get the number of threads.
	//!
	inline int GetThreadCount(void) const
	{
		return static_cast< int >(m_Threads.size());
	}

	//!
	//! Set the number of threads.
	//!
	//! @param count
	//!		Number of threads to use. 0 will execute tasks instantly, -1 will create
	//!		threads depending on the hardware's capabilities.
	//!
	//! @param wait
	//!		If true, the manager will wait until every task is processed, otherwise
	//!		it will discard the queued tasks (it will still wait for the running ones)
	//!
	inline void SetThreadCount(int count, bool wait)
	{
		// ensure we have a valid thread count
		if (count < 0)
		{
			count = std::thread::hardware_concurrency();
		}

		// do nothing if the number of threads is already correct
		if (count == this->GetThreadCount())
		{
			return;
		}

		// ensure we're not pushing new tasks
		m_Stop = true;

		// clear the queue if we don't want to wait
		if (wait == false)
		{
			this->EmptyQueue();
		}

		// wait until the processing tasks are completed
		this->Wait();

		// join threads
		m_ConditionVariable.notify_all();
		this->JoinThreads();

		// clear previous data and reserve space for new ones
		m_Threads.clear();
		m_Threads.reserve(count);
		m_ThreadLocalStorage.clear();
		m_ThreadLocalStorage.resize(count == 0 ? 1 : count);
		memset(m_ThreadLocalStorage.data(), 0, count * sizeof(void *));

		// restore the stop atomic
		m_Stop = false;

		// create the data
		for (int i = 0; i < count; ++i)
		{
			// create the thread
			m_Threads.emplace_back(std::thread([this, i] (void) {
				// the loop should be going on until the task manager wants to stop, or
				// we still have tasks to process.
				while (m_Stop == false || m_QueuedTaskCount > 0)
				{
					// lock the queue's mutex to check if there is a task to do
					m_QueueMutex.lock();

					// check if we have tasks
					if (m_Queue.empty())
					{
						// No task. Allow new ones to be queued
						m_QueueMutex.unlock();

						// and wait for something to do (we use a predicate to only wait
						// if the task manager has not been stopped)
						std::unique_lock< std::mutex > uniqueLock(m_ConditionVariableMutex);
						m_ConditionVariable.wait(uniqueLock);

						// once activated, we want to go back to the beginning of the loop
						// to lock again the queue's mutex and check the queue, etc.
						continue;
					}
					else
					{
						// we've got a task ! Get it, pop it, and release the queue's lock.
						Task task(std::move(m_Queue.front()));
						m_Queue.pop();
						--m_QueuedTaskCount;
						++m_RunningTaskCount;
						m_QueueMutex.unlock();

						// and execute it
						task(m_ThreadLocalStorage[i]);

						// when done executing, decrease the task count
						--m_RunningTaskCount;
					}
				}
			}));
		}
	}

	//!
	//! Wait until there is no longer any task queued or running.
	//! This will stall the current thread.
	//!
	inline void Wait(void)
	{
		while (m_RunningTaskCount > 0 || m_QueuedTaskCount > 0)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}
	}

	//!
	//! Cancel every queued tasks and stalls the current thread until the currently
	//! running ones are completed.
	//!
	inline void Cancel(void)
	{
		// clear the task queue
		{
			std::lock_guard< std::mutex > lock(m_QueueMutex);
			this->EmptyQueue();
		}

		// and wait for running ones
		this->Wait();
	}

private:

	//!
	//! Empty the queue. This will not lock the queue, use carefully.
	//!
	inline void EmptyQueue(void)
	{
		while (m_Queue.empty() == false)
		{
			m_Queue.pop();
			--m_QueuedTaskCount;
		}
	}

	//!
	//! Join threads
	//!
	inline void JoinThreads(void)
	{
		// join threads
		for (std::thread & thread : m_Threads)
		{
			thread.join();
		}
	}

	//! The list of threads
	std::vector< std::thread > m_Threads;

	//! The jobs
	std::queue< Task > m_Queue;

	//! The mutex used to protect the job queue
	std::mutex m_QueueMutex;

	//! Condition variable used to awake the threads when a job is available
	std::condition_variable m_ConditionVariable;

	//! Mutex used with the condition variable
	std::mutex m_ConditionVariableMutex;

	//! Used to stop the threads' loop before joining them
	std::atomic_bool m_Stop;

	//! Fake thread local storage
	std::vector< void * > m_ThreadLocalStorage;

	//! Number of tasks in the queue
	std::atomic_int m_QueuedTaskCount;

	//! Number of running tasks
	std::atomic_int m_RunningTaskCount;

};


#endif // TASK_MANAGER_H
