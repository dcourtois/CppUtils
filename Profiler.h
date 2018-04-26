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
	inline int64_t GetNanoSeconds(TimePoint start, TimePoint end)
	{
		return std::chrono::duration_cast< std::chrono::nanoseconds >(end - start).count();
	}

	//!
	//! Get the time elapsed between 2 time points in microseconds
	//!
	inline int64_t GetMicroSeconds(TimePoint start, TimePoint end)
	{
		return std::chrono::duration_cast< std::chrono::microseconds >(end - start).count();
	}

	//!
	//! Get the time elapsed between 2 time points in milliseconds
	//!
	inline int64_t GetMilliSeconds(TimePoint start, TimePoint end)
	{
		return std::chrono::duration_cast< std::chrono::milliseconds >(end - start).count();
	}

#if PROFILER_ENABLE == 1

	// forward definitions
	struct MarkerList;
	struct MarkerData;
	struct ScopeData;

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

		//! The ID of the scope
		uint32_t ScopeID;

		//! Thread ID
		ThreadID ID;

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
	inline uint32_t RegisterScope(String && name, String && filename, uint32_t line)
	{
		ScopedLock< Mutex > lock(ScopeMutex);
		Scopes.push_back({ name, filename, line });
		return static_cast< uint32_t >(Scopes.size() - 1);
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
		inline ProfileScope(uint32_t scopeID)
			: m_ScopeID(scopeID)
			, m_ThreadID(GetCurrentThreadID())
			, m_Start(GetCurrentTime())
		{
		}

		//!
		//! Destructor
		//!
		inline ~ProfileScope(void)
		{
			Markers.Data->push_back({
				m_ScopeID,
				m_ThreadID,
				m_Start,
				GetCurrentTime()
			});
		}

	private:

		//! Scope ID
		uint32_t m_ScopeID;

		//! Thread ID
		ThreadID m_ThreadID;

		//! Starting point
		TimePoint m_Start;

	};

#endif // PROFILER_ENABLE == 1


} // namespace Profiler


#if PROFILER_ENABLE == 1


//!
//! @def BEGIN_PROFILE(id)
//!
//! This macro is used with #END_PROFILE(id) to profile a specific piece of code.
//!
#define BEGIN_PROFILE(id)																			\
	static const uint32_t _scope_id_##id = Profiler::RegisterScope(#id, __FILE__, __LINE__);		\
	Profiler::ThreadID _thread_id_##id = Profiler::GetCurrentThreadID();							\
	Profiler::TimePoint _start_##id = Profiler::GetCurrentTime();

//!
//! @def END_PROFILE(id)
//!
//! Used with #BEGIN_PROFILE(id) to profile a specific piece of code.
//!
#define END_PROFILE(id)				\
	Profiler::AddMarker({			\
		_scope_id_##id,				\
		_thread_id_##id,			\
		_start_##id,				\
		Profiler::GetCurrentTime()	\
	})

//!
//! @def PROFILE_FUNCTION
//!
//! Use this macro at the begining of a function to profile it.
//!
//! @see #PROFILE_SCOPE
//!
#if defined(_MSC_VER)
#	define PROFILE_FUNCTION() PROFILE_SCOPE(__FUNCTION__)
#else
#	define PROFILE_FUNCTION() PROFILE_SCOPE(__func__)
#endif

//!
//! @def PROFILE_SCOPE
//!
//! Profile the current scope : will profile what happens between the
//! use of this macro and the end of the scope it was used in.
//!
#define PROFILE_SCOPE(name)																								\
	static const uint32_t PRIVATE_MERGE(_scope_id_, __LINE__) = Profiler::RegisterScope(name, __FILE__, __LINE__);		\
	Profiler::ProfileScope PRIVATE_MERGE(_profile_, __LINE__)(PRIVATE_MERGE(_scope_id_, __LINE__))

//!
//! Private macro used to merge 2 values.
//!
#define PRIVATE_MERGE_2(a, b) a##b

//!
//! Private macro used to merge 2 values. See #PRIVATE_MERGE_2
//!
#define PRIVATE_MERGE(a, b) PRIVATE_MERGE_2(a, b)

#include "ProfilerOutput.h"

#else

#	define BEGIN_PROFILE(id)		(void)0
#	define END_PROFILE(id)			(void)0
#	define PROFILE_FUNCTION()		(void)0
#	define PROFILE_SCOPE(name)		(void)0

#endif

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

} // namespace Profiler

#endif // PROFILER_IMPLEMENTATION
