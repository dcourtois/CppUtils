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



#if MEMORY_CHECK == 1


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
	//! Enable/Disable memory tracking
	//!
	inline static void SetEnabled(bool enabled)
	{
		s_Enabled = enabled;
	}

	//!
	//! Check if the tracker is enabled
	//!
	inline static bool IsEnabled(void)
	{
		return s_Enabled;
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
	//! Get the tracked memory size in bytes.
	//!
	inline uint64_t GetTrackedMemory(void)
	{
		std::lock_guard< std::mutex > lock(m_Mutex);
		uint64_t size = 0;
		for (const auto & chunk : m_Chunks)
		{
			size += std::get< 0 >(chunk.second);
		}
		return size;
	}

	//!
	//! Get the tracked memory chunks.
	//!
	inline AllocatedBlockMap GetTrackedChunks(void)
	{
		std::lock_guard< std::mutex > lock(m_Mutex);
		return m_Chunks;
	}

	//!
	//! Track a chunk of memory
	//!
	inline void Track(void *pointer, size_t size, const char *filename, int line)
	{
		if (s_Enabled == true)
		{
			std::lock_guard< std::mutex > lock(m_Mutex);

			// break if the allocation count was set, and it's the current one
			if (m_BreakOnAlloc == m_AllocationCount)
			{
				int *dead = nullptr;
				*dead = 0;
			}

			// update the chunks
			m_Chunks.insert(std::make_pair(pointer, Chunk(size, filename, line, m_AllocationCount++)));
		}
	}

	//!
	//! Untrack a chunk of memory
	//!
	inline void Untrack(void *pointer)
	{
		if (s_Enabled == true)
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
	{
	}

	//!
	//! Destructor
	//!
	~MemoryTracker(void)
	{
		s_Enabled = false;
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
	static bool s_Enabled;

	//! the global instance
	static MemoryTracker s_Instance;

};

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

//
// override delete, but this cannot be done inline
//
void operator delete (void * pointer) noexcept;
void operator delete [] (void * pointer) noexcept;


//!
//! @def MT_NEW
//!		Use instead of `new` to track the allocated chunk of memory. Use @a MT_DELETE
//!		to release memory allocated through @MT_NEW.
//!
#define MT_NEW new (__FILE__, __LINE__)

//!
//! @def MT_DELETE
//!		Use instead of `delete` to release memory allocated through through @MT_NEW.
//!
#define MT_DELETE delete

//!
//! @def MT_SET_ENABLED(enabled)
//!		Use this macro to enable/disable memory tracking.
//!
#define MT_SET_ENABLED(enabled) MemoryTracker::GetInstance().SetEnabled(enabled)

//!
//! @def MT_INIT_MEM_TRACKER
//!		This will reset all tracked memory. This can be used to avoid tracking statically
//!		allocated stuff.
//!
#define MT_INIT_MEM_TRACKER() MemoryTracker::GetInstance().Clear()

//!
//! @def MT_BREAK_ON_ALLOC(count)
//!		Use this macro to tell the memory tracker to break when it reaches @p count
//!		allocation.
//!
#define MT_BREAK_ON_ALLOC(count) MemoryTracker::GetInstance().BreakOnAlloc(count)

//!
//! @def MT_GET_ALLOCATED_MEMORY
//!		Get the currently tracked allocated memory in bytes.
//!
#define MT_GET_ALLOCATED_MEMORY() MemoryTracker::GetInstance().GetTrackedMemory()

//!
//! @def MT_GET_ALLOCATED_CHUNKS
//!		Get the currently allocated chunks of memory.
//!
#define MT_GET_ALLOCATED_CHUNKS() MemoryTracker::GetInstance().GetTrackedChunks()

#else

#	define MT_NEW						new
#	define MT_DELETE					delete
#	define MT_SET_ENABLED(enabled)		(void)0
#	define MT_INIT_MEM_TRACKER()		(void)0
#	define MT_BREAK_ON_ALLOC(count)		(void)0
#	define MT_GET_ALLOCATED_MEMORY()	0
#	define MT_GET_ALLOCATED_CHUNKS()	0

#endif


#endif // MEMORY_TRACKER


//!
//! If @a MEMORY_TRACKER_IMPLEMENTATION is defined, define data.
//!
#if defined(MEMORY_TRACKER_IMPLEMENTATION) && MEMORY_CHECK == 1

//!
//! Override the delete operator
//!
void operator delete (void * pointer) noexcept
{
	MemoryTracker::GetInstance().Untrack(pointer);
	free(pointer);
}

//!
//! Override the delete array operator
//!
void operator delete [] (void * pointer) noexcept
{
	MemoryTracker::GetInstance().Untrack(pointer);
	free(pointer);
}

// the memory tracker instance
MemoryTracker	MemoryTracker::s_Instance;
bool			MemoryTracker::s_Enabled = true;

#endif
