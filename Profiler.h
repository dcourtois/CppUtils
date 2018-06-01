#ifndef PROFILER_H
#define PROFILER_H

//!
//! Use @a PROFILER_ENABLE to explicitely enable or disable the profiling macros.
//! If not defined by the user, we will define it to 1
//!
#if !defined(PROFILER_ENABLE)
#	define PROFILER_ENABLE 1
#endif


#include <chrono>
#include <mutex>
#include <string>
#include <thread>
#include <vector>


//!
//! Fix for f***ing Windows headers defining GetCurrentTime ...
//!
#if defined(GetCurrentTime)
#	undef GetCurrentTime
#endif


namespace Profiler
{

	//! Define a precise time point
	typedef std::chrono::high_resolution_clock::time_point TimePoint;

	//! Defines a thread ID
	typedef std::thread::id ThreadID;

	//!
	//! Get the current thread ID
	//!
	inline ThreadID GetCurrentThreadID(void)
	{
		return std::this_thread::get_id();
	}

	//!
	//! Get the current time
	//!
	inline TimePoint GetCurrentTime(void)
	{
		return std::chrono::high_resolution_clock::now();
	}

	//!
	//! Get the time elapsed between 2 time points in nanoseconds
	//!
	inline uint64_t GetNanoSeconds(TimePoint start, TimePoint end)
	{
		return std::chrono::duration_cast< std::chrono::nanoseconds >(end - start).count();
	}

	//!
	//! Get the time elapsed between 2 time points in microseconds
	//!
	inline uint64_t GetMicroSeconds(TimePoint start, TimePoint end)
	{
		return std::chrono::duration_cast< std::chrono::microseconds >(end - start).count();
	}

	//!
	//! Get the time elapsed between 2 time points in milliseconds
	//!
	inline uint64_t GetMilliSeconds(TimePoint start, TimePoint end)
	{
		return std::chrono::duration_cast< std::chrono::milliseconds >(end - start).count();
	}

#if PROFILER_ENABLE == 1

	// forward definitions
	struct MarkerList;
	struct MarkerData;
	struct ScopeData;

	//! Define a scope ID
	typedef uint16_t ScopeID;

	//! Define a string
	typedef std::string String;

	//! Define a mutex
	typedef std::mutex Mutex;

	//! Define scope based lock on a mutex
	template < typename Type > using ScopedLock = std::lock_guard< Type >;

	//! Define a dynamically allocated vector
	template < typename Type > using Vector = std::vector< Type >;

	//! The profiling start point
	extern const TimePoint Start;

	//! The list of registered scopes
	extern Vector< ScopeData > Scopes;

	//! Mutex to protect scope vector access
	extern Mutex ScopeMutex;

	//! The global marker lists
	extern Vector< Vector< MarkerData > * > MarkerLists;

	//! Mutex to protect Markers access
	extern Mutex MarkerMutex;

	//! The "per-thread" marker lists
	extern thread_local MarkerList Markers;

	//! The per-thread current scope ID
	extern thread_local ScopeID CurrentScope;

	//!
	//! Define a scope data
	//!
	struct ScopeData
	{

		//! Name of the scope
		String Name;

		//! The source file
		String Filename;

		//! The line
		uint32_t Line;

	};

	//!
	//! A raw input data
	//!
	struct MarkerData
	{

		//! The ID of the parent scope
		ScopeID ParentScope;

		//! The ID of the scope
		ScopeID Scope;

		//! Thread ID
		ThreadID Thread;

		//! Start point
		TimePoint Start;

		//! End point
		TimePoint End;

	};

	//!
	//! Structure used to register per-thread marker lists to the global markers one.
	//!
	struct MarkerList
	{

		//!
		//! Constructor. This will allocate a vector of MarkerData, and register it
		//! to the global list of marker lists
		//!
		inline MarkerList(void)
			: Data(new Vector< MarkerData >())
		{
			ScopedLock< Mutex > lock(MarkerMutex);
			MarkerLists.push_back(Data);
		}

		//! The markers
		Vector< MarkerData > * Data;

	};

	//!
	//! Register a new scope
	//!
	inline ScopeID RegisterScope(String && name, String && filename, uint32_t line)
	{
		ScopedLock< Mutex > lock(ScopeMutex);
		Scopes.push_back({ name, filename, line });
		return static_cast< ScopeID >(Scopes.size() - 1);
	}

	//!
	//! Scope based profile
	//!
	class ProfileScope
	{

	public:

		//!
		//! Constructor
		//!
		inline ProfileScope(ScopeID scope)
			: m_ParentScope(CurrentScope)
			, m_Scope(scope)
			, m_Thread(GetCurrentThreadID())
			, m_Start(GetCurrentTime())
		{
			// update the current scope
			CurrentScope = scope;
		}

		//!
		//! Destructor
		//!
		inline ~ProfileScope(void)
		{
			// add the marker
			Markers.Data->push_back({
				m_ParentScope,
				m_Scope,
				m_Thread,
				m_Start,
				GetCurrentTime()
			});

			// restore the previous scope
			CurrentScope = m_ParentScope;
		}

	private:

		//! Parent scope ID
		ScopeID m_ParentScope;

		//! Scope ID
		ScopeID m_Scope;

		//! Thread ID
		ThreadID m_Thread;

		//! Starting point
		TimePoint m_Start;

	};

#endif // PROFILER_ENABLE == 1


} // namespace Profiler


#if PROFILER_ENABLE == 1


//!
//! @def PROFILE_FUNCTION
//!
//! Use this macro at the begining of a function to profile it.
//!
//! @see #PROFILE_SCOPE
//!
#if defined(_MSC_VER)
#	define PROFILE_FUNCTION() PROFILE_SCOPE(__FUNCTION__)
#elif defined(__GNUC__)
#	define PROFILE_FUNCTION() PROFILE_SCOPE(__PRETTY_FUNCTION__)
#else
#	define PROFILE_FUNCTION() PROFILE_SCOPE(__func__)
#endif

//!
//! @def PROFILE_SCOPE(name)
//!
//! Profile the current scope : will profile what happens between the
//! use of this macro and the end of the scope it was used in.
//!
#define PROFILE_SCOPE(name)																									\
	static const Profiler::ScopeID PRIVATE_MERGE(_scope_id_, __LINE__) = Profiler::RegisterScope(name, __FILE__, __LINE__);	\
	Profiler::ProfileScope PRIVATE_MERGE(_profile_, __LINE__)(PRIVATE_MERGE(_scope_id_, __LINE__))

//!
//! Private macro used to merge 2 values.
//!
#define PRIVATE_MERGE_2(a, b) a##b

//!
//! Private macro used to merge 2 values. See #PRIVATE_MERGE_2
//!
#define PRIVATE_MERGE(a, b) PRIVATE_MERGE_2(a, b)


#else // PROFILER_ENABLE == 0


#	define PROFILE_FUNCTION()		(void)0
#	define PROFILE_SCOPE(name)		(void)0


#endif // PROFILER_ENABLE


//
// convenience output functions
//
#include "ProfilerOutput.h"


#endif // PROFILER_H


//!
//! If @a PROFILER_IMPLEMENTATION is defined, define data.
//!
#if defined(PROFILER_IMPLEMENTATION) && PROFILER_ENABLE == 1

namespace Profiler
{

	const TimePoint Start = GetCurrentTime();
	Vector< ScopeData > Scopes;
	Mutex ScopeMutex;
	Vector< Vector< MarkerData > * > MarkerLists;
	Mutex MarkerMutex;
	thread_local MarkerList Markers;
	thread_local ScopeID CurrentScope = static_cast< ScopeID >(-1);

} // namespace Profiler

#endif // PROFILER_IMPLEMENTATION
