#ifndef PROFILER_H
#define PROFILER_H


#include <chrono>
#include <mutex>
#include <string>
#include <thread>
#include <vector>


//!
//! Use @a PROFILER_ENABLE to explicitely enable or disable the profiling macros.
//! If not defined by the user, we will define it to 1
//!
#if !defined(PROFILER_ENABLE)
#	define PROFILER_ENABLE 1
#endif


//!
//! Fix for f***ing Windows headers defining GetCurrentTime ...
//!
#if defined(GetCurrentTime)
#	undef GetCurrentTime
#endif


namespace Profiler
{

	// forward definitions
	struct MarkerData;
	struct ScopeData;

	//! Define a precise time point
	typedef std::chrono::high_resolution_clock::time_point TimePoint;

	//! Defines a thread ID
	typedef std::thread::id ThreadID;

	//! The profiling start point
	extern TimePoint Start;

	//! The list of registered scopes
	extern std::vector< ScopeData > Scopes;

	//! Mutex to protect scope vector access
	extern std::mutex ScopeMutex;

	//! The markers
	extern std::vector< MarkerData > Markers;

	//! Mutex to protect marker vector access
	extern std::mutex MarkerMutex;

	//! Enabled state
	extern bool Enabled;

	//!
	//! Define a scope data
	//!
	struct ScopeData
	{

		//! Name of the scope
		std::string Name;

		//! The source file
		std::string Filename;

		//! The line
		std::size_t Line;

	};

	//!
	//! A raw input data
	//!
	struct MarkerData
	{

		//! The ID of the scope
		std::size_t ScopeID;

		//! Thread ID
		ThreadID ID;

		//! Start point
		TimePoint Start;

		//! End point
		TimePoint End;

	};

	//!
	//! Register a new scope
	//!
	inline std::size_t RegisterScope(std::string && name, std::string && filename, std::size_t line)
	{
		std::lock_guard< std::mutex > lock(ScopeMutex);
		Scopes.push_back({ name, filename, line });
		return Scopes.size() - 1;
	}

	//!
	//! Add a new marker
	//!
	inline void AddMarker(MarkerData && marker)
	{
		std::lock_guard< std::mutex > lock(MarkerMutex);
		Markers.push_back(std::move(marker));
	}

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

	//!
	//! Scope based profile
	//!
	class ProfileScope
	{

	public:

		//!
		//! Constructor
		//!
		inline ProfileScope(std::size_t scopeID)
			: m_ScopeID(scopeID)
			, m_ThreadID(GetCurrentThreadID())
			, m_Start(GetCurrentTime())
		{
		}

		//!
		//! Destructor
		//!
		~ProfileScope(void)
		{
			if (Enabled == true)
			{
				AddMarker({
					m_ScopeID,
					m_ThreadID,
					m_Start,
					GetCurrentTime()
				});
			}
		}

	private:

		//! Scope ID
		std::size_t m_ScopeID;

		//! Thread ID
		ThreadID m_ThreadID;

		//! Starting point
		TimePoint m_Start;

	};

//
// Define profiling macros
//
#if PROFILER_ENABLE == 1

	//!
	//! @def BEGIN_PROFILE(id)
	//!
	//! This macro is used with #END_PROFILE(id) to profile a specific piece of code.
	//!
#	define BEGIN_PROFILE(id)																			\
		static const std::size_t _scope_id_##id = Profiler::RegisterScope(#id, __FILE__, __LINE__);		\
		Profiler::ThreadID _thread_id_##id = Profiler::GetCurrentThreadID();							\
		Profiler::TimePoint _start_##id = Profiler::GetCurrentTime();

	//!
	//! @def END_PROFILE(id)
	//!
	//! Used with #BEGIN_PROFILE(id) to profile a specific piece of code.
	//!
#	define END_PROFILE(id)				\
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
#	define PROFILE_FUNCTION() PROFILE_SCOPE(__func__)

	//!
	//! @def PROFILE_SCOPE
	//!
	//! Profile the current scope : will profile what happens between the
	//! use of this macro and the end of the scope it was used in.
	//!
#	define PROFILE_SCOPE(name)																								\
		static const std::size_t PRIVATE_MERGE(_scope_id_, __LINE__) = Profiler::RegisterScope(name, __FILE__, __LINE__);	\
		Profiler::ProfileScope PRIVATE_MERGE(_profile_, __LINE__)(PRIVATE_MERGE(_scope_id_, __LINE__))

	//!
	//! Private macro used to merge 2 values.
	//!
#	define PRIVATE_MERGE_2(a, b) a##b

	//!
	//! Private macro used to merge 2 values. See #PRIVATE_MERGE_2
	//!
#	define PRIVATE_MERGE(a, b) PRIVATE_MERGE_2(a, b)

#else
#	define BEGIN_PROFILE(id)
#	define END_PROFILE(id)
#	define PROFILE_FUNCTION()
#	define PROFILE_SCOPE(name)
#endif


} // namespace Profiler


#endif // PROFILER_H

//!
//! If @a PROFILER_IMPLEMENTATION is defined, define data.
//!
#if defined(PROFILER_IMPLEMENTATION)

namespace Profiler
{

	TimePoint Start = GetCurrentTime();
	std::vector< ScopeData > Scopes;
	std::mutex ScopeMutex;
	std::vector< MarkerData > Markers;
	std::mutex MarkerMutex;
	bool Enabled;

} // namespace Profiler

#endif // PROFILER_IMPLEMENTATION
