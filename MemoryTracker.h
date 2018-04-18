#ifndef MEMORY_TRACKER
#define MEMORY_TRACKER


//!
//! \a MEMORY_CHECK
//!
//! By default, memory tracking and leak reporting is only done in
//! debug. Set this macro to 1 to always activate this, or to 0 to
//! always disable it.
//!
#if !defined(MEMORY_CHECK)
#	if (RELEASE)
#		define MEMORY_CHECK 0
#	else
#		define MEMORY_CHECK 1
#	endif
#endif

#include <unordered_map>
#include <mutex>

//!
//! Allocator used with our hash map to avoid recursive locks
//!
template< class T >
struct Allocator
{
	//! Value of the allocated objects (required)
	typedef T value_type;

	//! Default constructor (required)
	Allocator(void)
	{
	}

	//! Copy (?) constructor (required)
	template<class U> Allocator(const Allocator< U > &)
	{
	}

	//! Allocate a few objects.
	inline T *allocate(std::size_t num)
	{
		return reinterpret_cast< T * >(malloc(num * sizeof(T)));
	}

	//! De-allocate memory.
	inline void deallocate(T * pointer, std::size_t)
	{
		free(pointer);
	}
};

//! Global equality operator for our allocator (required)
template< class T, class U > inline bool operator == (const Allocator< T >&, const Allocator< U >&) { return false; }

//! Global inequality operator for our allocator (required)
template< class T, class U > inline bool operator != (const Allocator< T >&, const Allocator< U >&) { return true; }

//!
//! Mem tracker class.
//!
class MemoryTracker
{

public:

	//!
	//! Define a chunk of tracked memory
	//! - number of bytes
	//! - filename
	//! - line
	//! - allocation number (can be used with BreakOnAlloc)
	//!
	typedef std::tuple<size_t, const char *, int, int64_t> Chunk;

	//!
	//! Define our custom hash map (using a malloc/free allocator to avoid recursive new)
	//!
	typedef std::unordered_map<
		void *,
		Chunk,
		std::hash< void * >,
		std::equal_to< void * >,
		Allocator<std::pair< void * const, Chunk > >
	> AllocatedBlockMap;

	//!
	//! singleton implementation
	//!
	inline static MemoryTracker& GetInstance(void)
	{
		return s_Instance;
	}

	//!
	//! Reset everything
	//!
	inline void Clear(void)
	{
		std::lock_guard< std::mutex > lock(m_Mutex);
		m_AllocationCount = 0;
		m_BreakOnAlloc = -1;
		m_Chunks.clear();
	}

	//!
	//! Set the allocation break count
	//!
	//! \note
	//!		This is not protected by a mutex lock since it should be used only
	//!		to debug a specific allocation, and thus should not appear more than
	//!		once in the code. Moreover, this should be done just avec having
	//!		initialized the memory tracker.
	//!
	inline void BreakOnAlloc(int64_t count)
	{
		m_BreakOnAlloc = count;
	}

	//!
	//! Get the tracked memory chunks.
	//!
	inline AllocatedBlockMap GetTrackedMemory(void) const
	{
		return m_Chunks;
	}

	//!
	//! Track a chunk of memory
	//!
	inline void Track(void *pointer, size_t size, const char *filename, int line)
	{
		if (m_Enabled == true)
		{
			std::lock_guard< std::mutex > lock(m_Mutex);

			// break if the allocation count was set, and it's the current one
			if (m_BreakOnAlloc == m_AllocationCount)
			{
				int *dead = nullptr;
				*dead = 0;
			}

			// update the chunks
			m_Chunks.insert(std::make_pair(pointer, Chunk({ size, filename, line, m_AllocationCount++ })));
		}
	}

	//!
	//! Untrack a chunk of memory
	//!
	inline void Untrack(void *pointer)
	{
		if (m_Enabled == true)
		{
			std::lock_guard<std::mutex> lock(m_Mutex);
			auto entry = m_Chunks.find(pointer);
			if (entry != m_Chunks.end())
			{
				m_Chunks.erase(entry);
			}
		}
	}

private:

	//!
	//! Constructor
	//!
	MemoryTracker(void)
		: m_AllocationCount(0)
		, m_BreakOnAlloc(-1)
		, m_Enabled(true)
	{
	}

	//! mutex used to protect multithreaded access
	std::mutex m_Mutex;

	//! the current allocation counter
	int64_t m_AllocationCount;

	//! if different from -1, the mem tracker will break on this allocation
	int64_t m_BreakOnAlloc;

	//! our allocated chunks
	AllocatedBlockMap m_Chunks;

	//! enable the tracking
	bool m_Enabled;

	//! the global instance
	static MemoryTracker s_Instance;

};

#if MEMORY_CHECK == 1

//!
//! Override of new
//!
inline void * operator new (size_t size, const char * filename, int line)
{
	void * pointer = malloc(size);
	MemoryTracker::GetInstance().Track(pointer, size, filename, line);
	return pointer;
}

//!
//! Override of new []
//!
inline void * operator new [] (size_t size, const char * filename, int line)
{
	void * pointer = malloc(size);
	MemoryTracker::GetInstance().Track(pointer, size, filename, line);
	return pointer;
}

//!
//! This one is needed to fix C4291 on Windows
//!
inline void operator delete (void * pointer, const char * /* filename */, int /* line */)
{
	MemoryTracker::GetInstance().Untrack(pointer);
	free(pointer);
}

//!
//! This one is needed to fix C4291 on Windows
//!
inline void operator delete [] (void * pointer, const char * /* filename */, int /* line */)
{
	MemoryTracker::GetInstance().Untrack(pointer);
	free(pointer);
}

// global delete override. These are defined in the #if defined(MEMORY_TRACKER_IMPLEMENTATION)
// because they cannot be declared as inline.
void operator delete (void * pointer) noexcept;
void operator delete [] (void * pointer) noexcept;

#	define MT_NEW						new (__FILE__, __LINE__)
#	define MT_DELETE					delete
#	define MT_INIT_MEM_TRACKER()		MemoryTracker::GetInstance().Clear()
#	define MT_BREAK_ON_ALLOC(count)		MemoryTracker::GetInstance().BreakOnAlloc(count)
#	define MT_GET_ALLOCATED_MEM()		MemoryTracker::GetInstance().GetTrackedMemory()

#else

#	define MT_NEW						new
#	define MT_DELETE					delete
#	define MT_INIT_MEM_TRACKER()		(void)0
#	define MT_BREAK_ON_ALLOC(count)		(void)0
#	define MT_GET_ALLOCATED_MEM()		0

#endif


#endif // MEMORY_TRACKER


//!
//! If @a MEMORY_TRACKER_IMPLEMENTATION is defined, define data.
//!
#if defined(MEMORY_TRACKER_IMPLEMENTATION)

#	if MEMORY_CHECK == 1

//!
//! Override of delete
//!
void operator delete (void * pointer) noexcept
{
	MemoryTracker::GetInstance().Untrack(pointer);
	free(pointer);
}

//!
//! Override of delete []
//!
void operator delete [] (void * pointer) noexcept
{
	MemoryTracker::GetInstance().Untrack(pointer);
	free(pointer);
}

#	endif

// the memory tracker instance
MemoryTracker MemoryTracker::s_Instance;

#endif
