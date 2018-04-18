#ifndef TASK_MANAGER_H
#define TASK_MANAGER_H


#include <cassert>
#include <vector>
#include <condition_variable>
#include <queue>
#include <atomic>
#include <thread>
#include <mutex>


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
	{
		// get the number of threads supported by the platform if no threadCount
		// was provided.
		if (threadCount == -1)
		{
			threadCount = std::thread::hardware_concurrency();
		}

		// create the data
		for (int i = 0; i < threadCount; ++i)
		{
			// create the thread local storage
			m_ThreadLocalStorage.emplace_back(nullptr);

			// create the thread
			m_Threads.emplace_back(std::thread([this, i] (void) {
				// the loop should be going on until the task manager wants to stop, or
				// we still have tasks to process.
				while (m_Stop == false || this->HasTask() == true)
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
						m_ConditionVariable.wait(uniqueLock, [&] (void) -> bool { return m_Stop.load() == false; });

						// once activated, we want to go back to the beginning of the loop
						// to lock again the queue's mutex and check the queue, etc.
						continue;
					}
					else
					{
						// we've got a task ! Get it, pop it, and release the queue's lock.
						Task task(std::move(m_Queue.front()));
						m_Queue.pop();
						m_QueueMutex.unlock();

						// and execute it
						task(m_ThreadLocalStorage[i]);
					}
				}
			}));
		}
	}

	//!
	//! Destructor. Cleanup the threads. This will stall the current
	//! thread until all jobs are done processing and all threads have
	//! been correctly stopped and cleaned up.
	//!
	~TaskManager(void)
	{
		// notify the threads that they should stop
		m_Stop = true;

		// make sure threads are awake
		m_ConditionVariable.notify_all();

		// join threads
		for (std::thread & thread : m_Threads)
		{
			thread.join();
		}
	}

	//!
	//! Checks if there is a task queued.
	//!
	inline bool HasTask(void)
	{
		std::lock_guard< std::mutex > lock(m_QueueMutex);
		return m_Queue.empty() == false;
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
		assert(threadIndex >= 0 && threadIndex < static_cast< int >(m_Threads.size()));
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
		if (m_Threads.empty() == true)
		{
			// no jobs, just execute the task
			task(nullptr);
		}
		else
		{
			// push the job
			{
				std::lock_guard< std::mutex > lock(m_QueueMutex);
				m_Queue.emplace(task);
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
	//! Get the number of threads. The number of threads is specified
	//! in the constructor of the TaskManager and can't be changed
	//! afterwards.
	//!
	inline int GetNumThreads(void) const
	{
		return static_cast< int >(m_Threads.size());
	}

private:

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

};


#endif // TASK_MANAGER_H
