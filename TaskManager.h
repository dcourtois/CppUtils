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
		: m_State(State::Stopping)
		, m_QueuedTaskCount(0)
		, m_RunningThreadCount(0)
	{
		// init the threads
		this->SetThreadCount(threadCount);
	}

	//!
	//! Destructor. Cleanup the threads. This will stall the current
	//! thread until all jobs are done processing and all threads have
	//! been correctly stopped and cleaned up.
	//!
	~TaskManager(void)
	{
		m_State = State::Stopping;
		this->JoinThreads();
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
		// do nothing if we're stopping
		if (m_State == State::Stopping)
		{
			return;
		}

		// set the data
		assert(threadIndex >= 0 && threadIndex < static_cast< int >(m_ThreadLocalStorage.size()));
		m_ThreadLocalStorage[threadIndex] = data;
	}

	//!
	//! Push a new task. The task can be anything that is castable to
	//! a `std::function< void (void *) >`. Implicit cast will work with
	//! lambdas and static methods / functions.
	//!
	//! The `void *` parameter passed to the task is a pointer to some
	//! user data (use SetThreadLocalStorage to set this) that is shared
	//! per-thread.
	//!
	inline void PushTask(Task && task)
	{
		// do nothing if we're not running
		if (m_State != State::Running)
		{
			return;
		}

		// check if we have some threads
		if (m_Threads.empty() == true)
		{
			// no jobs, just execute the task
			task(m_ThreadLocalStorage.front());
		}
		else
		{
			// push the job
			{
				std::lock_guard< std::mutex > lock(m_QueueMutex);
				m_Queue.emplace(std::move(task));
				++m_QueuedTaskCount;
			}

			// and notify one thread that we have a job to do
			m_ConditionVariable.notify_one();
		}
	}

	//!
	//! Get the number of threads.
	//!
	inline int GetThreadCount(void) const
	{
		return static_cast< int >(m_Threads.size());
	}

	//!
	//! Get the current number of queued tasks
	//!
	inline int GetTaskCount(void) const
	{
		return m_QueuedTaskCount;
	}

	//!
	//! Set the number of threads.
	//!
	//! @param count
	//!		Number of threads to use. 0 will execute tasks instantly, -1 will create
	//!		threads depending on the hardware's capabilities.
	//!
	inline void SetThreadCount(int count)
	{
		// ensure we have a valid thread count
		if (count < 0)
		{
			count = std::thread::hardware_concurrency();
		}

		// do nothing if the number of threads is already correct
		if (count == static_cast< int >(m_Threads.size()))
		{
			return;
		}

		// update the state of the manager
		m_State = State::Stopping;

		// clear the queue and wait until running ones are processed and join the threads
		this->EmptyQueue();
		this->JoinThreads();

		// clear previous data and reserve space for new ones
		m_RunningThreadCount = count;
		m_Threads.clear();
		m_Threads.reserve(count);
		m_ThreadLocalStorage.clear();
		m_ThreadLocalStorage.resize(count == 0 ? 1 : count);
		memset(m_ThreadLocalStorage.data(), 0, m_ThreadLocalStorage.size() * sizeof(void *));

		// set the state as paused while we start the threads
		m_State = State::Paused;

		// create the threads
		for (int i = 0; i < count; ++i)
		{
			m_Threads.emplace_back(std::thread([&, i] (void) {
				// the loop should be going on until the task manager wants to stop, or
				// we still have tasks to process.
				while (m_State != State::Stopping || m_QueuedTaskCount > 0)
				{
					// lock the queue's mutex to check if there is a task to do
					m_QueueMutex.lock();

					// check if we have tasks
					if (m_Queue.empty())
					{
						// No task. Allow new ones to be queued
						m_QueueMutex.unlock();

						// and wait for something to do
						std::unique_lock< std::mutex > uniqueLock(m_ConditionVariableMutex);
						--m_RunningThreadCount;
						m_ConditionVariable.wait(uniqueLock);
						++m_RunningThreadCount;
					}
					else
					{
						// we've got a task ! Get it, pop it, and release the queue's lock.
						Task task(std::move(m_Queue.front()));
						m_Queue.pop();
						--m_QueuedTaskCount;
						m_QueueMutex.unlock();

						// and execute it
						task(m_ThreadLocalStorage[i]);
					}
				}
			}));
		}

		// ok, we're good to go
		m_State = State::Running;
	}

	//!
	//! Wait until there is no longer any task queued or running.
	//! This will stall the current thread.
	//!
	//! @param us
	//!		Number of micro seconds to wait between 2 checks.
	//!
	inline void Wait(uint64_t us = 1000)
	{
		while (m_RunningThreadCount > 0)
		{
			std::this_thread::sleep_for(std::chrono::microseconds(us));
		}
	}

	//!
	//! Cancel every queued tasks and stall the current thread until the currently
	//! running ones are completed.
	//!
	inline void Cancel(uint64_t us = 1000)
	{
		m_State = State::Paused;
		this->EmptyQueue();
		this->Wait(us);
		m_State = State::Running;
	}

private:

	//!
	//! Empty the queue.
	//!
	inline void EmptyQueue(void)
	{
		std::lock_guard< std::mutex > lock(m_QueueMutex);
		while (m_Queue.empty() == false)
		{
			m_Queue.pop();
			--m_QueuedTaskCount;
		}
		assert(m_Queue.empty() == true && m_QueuedTaskCount == 0);
	}

	//!
	//! Join threads.
	//!
	inline void JoinThreads(void)
	{
		assert(m_State == State::Stopping);

		// no thread, easy
		if (m_Threads.empty() == true)
		{
			return;
		}

		// wait until everyone's asleep
		this->Wait();

		// join threads
		m_ConditionVariable.notify_all();
		for (std::thread & thread : m_Threads)
		{
			thread.join();
		}
	}

	//! The various states of the manager
	enum class State
		: int
	{
		//! Running
		Running = 0,

		//! Paused
		Paused,

		//! Stopping
		Stopping
	};

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

	//! The current state of the manager
	std::atomic< State > m_State;

	//! Fake thread local storage
	std::vector< void * > m_ThreadLocalStorage;

	//! Number of tasks in the queue
	std::atomic_int m_QueuedTaskCount;
	
	//! Number of running threads
	std::atomic_int m_RunningThreadCount;

};


#endif // TASK_MANAGER_H
