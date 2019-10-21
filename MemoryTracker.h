#ifndef MEMORY_TRACKER
#define MEMORY_TRACKER


//!
//! @a MEMORY_CHECK
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
#include <cinttypes>


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
	//!
	struct Chunk
	{
		//! Number of allocated bytes
		size_t Bytes;

		//! Filename where it was allocated
		const char * Filename;

		//! Line at which it was allocated
		int Line;

		//! The allocation ID. Use with the "break on allocation" feature.
		int64_t AllocationID;
	};

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
	inline static MemoryTracker& Instance(void)
	{
		static MemoryTracker instance;
		return instance;
	}

	//!
	//! Enable/Disable memory tracking
	//!
	inline void SetEnabled(bool enabled)
	{
		m_Enabled = enabled;
	}

	//!
	//! Check if the tracker is enabled
	//!
	inline bool IsEnabled(void)
	{
		return m_Enabled;
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
			size += chunk.second.Bytes;
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
			m_Chunks.insert(std::make_pair(pointer, Chunk{ size, filename, line, m_AllocationCount++ }));
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

	//!
	//! Destructor
	//!
	~MemoryTracker(void)
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

};

//!
//! Override of new
//!
inline void * operator new (size_t size, const char * filename, int line)
{
	void * pointer = malloc(size);
	MemoryTracker::Instance().Track(pointer, size, filename, line);
	return pointer;
}

//!
//! Override of new []
//!
inline void * operator new [] (size_t size, const char * filename, int line)
{
	void * pointer = malloc(size);
	MemoryTracker::Instance().Track(pointer, size, filename, line);
	return pointer;
}

//!
//! This one is needed to fix C4291 on Windows
//!
inline void operator delete (void * pointer, const char * /* filename */, int /* line */)
{
	MemoryTracker::Instance().Untrack(pointer);
	free(pointer);
}

//!
//! This one is needed to fix C4291 on Windows
//!
inline void operator delete [] (void * pointer, const char * /* filename */, int /* line */)
{
	MemoryTracker::Instance().Untrack(pointer);
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
#define MT_SET_ENABLED(enabled) MemoryTracker::Instance().SetEnabled(enabled)

//!
//! @def MT_INIT_MEM_TRACKER
//!		This will reset all tracked memory. This can be used to avoid tracking statically
//!		allocated stuff.
//!
#define MT_INIT_MEM_TRACKER() MemoryTracker::Instance().Clear()

//!
//! @def MT_BREAK_ON_ALLOCATION(count)
//!		Use this macro to tell the memory tracker to break when it reaches @p count
//!		allocation.
//!
#define MT_BREAK_ON_ALLOCATION(count) MemoryTracker::Instance().BreakOnAlloc(count)

//!
//! @def MT_GET_ALLOCATED_MEMORY
//!		Get the currently tracked allocated memory in bytes.
//!
#define MT_GET_ALLOCATED_MEMORY() MemoryTracker::Instance().GetTrackedMemory()

//!
//! @def MT_GET_ALLOCATED_CHUNKS
//!		Get the currently allocated chunks of memory.
//!
#define MT_GET_ALLOCATED_CHUNKS() MemoryTracker::Instance().GetTrackedChunks()

//!
//! @def MT_SHUTDOWN(log)
//!		This can be called when you're leaving your application to disable memory
//!		tracking, and logging a report that will tell you if there are still some
//!		memory chunks allocated or not, and if there are, where the allocation was.
//!		You just have to provide a @a log function that works like printf.
//!
#define MT_SHUTDOWN(log)																			\
	MemoryTracker::Instance().SetEnabled(false);													\
	auto chunks = MemoryTracker::Instance().GetTrackedChunks();										\
	if (chunks.empty() == true)																		\
	{																								\
		log("No memory leaks, congratulations !\n");												\
	}																								\
	else																							\
	{																								\
		size_t size = 0;																			\
		for (const auto & entry : chunks)															\
		{																							\
			size += entry.second.Bytes;																\
		}																							\
		log("Memory leak detected - %" PRIu64 " block%s (%" PRIu64 " byte%s) still allocated\n",	\
			chunks.size(), chunks.size() > 1 ? "s" : "",											\
			size, size > 1 ? "s" : ""																\
		);																							\
		for (const auto & entry : chunks)															\
		{																							\
			log(																					\
				"%s:%d: [%" PRId64 "] %" PRIu64 " bytes at location %p\n",							\
				entry.second.Filename,																\
				entry.second.Line,																	\
				entry.second.AllocationID,															\
				entry.second.Bytes,																	\
				entry.first																			\
			);																						\
		}																							\
	}


#else

#	define MT_NEW						new
#	define MT_DELETE					delete
#	define MT_SET_ENABLED(enabled)		(void)0
#	define MT_INIT_MEM_TRACKER()		(void)0
#	define MT_BREAK_ON_ALLOC(count)		(void)0
#	define MT_GET_ALLOCATED_MEMORY()	0
#	define MT_GET_ALLOCATED_CHUNKS()	0
#	define MT_SHUTDOWN(log)				(void)0

#endif

//!
//! @def MT_DECLARE_INLINE_NEW_OPERATOR
//!		Declare an inline class' operator new function, which will add the
//!		additional parameters needed for this class to be tracked by the memory
//!		tracker.
//!
//! @def MT_DECLARE_INLINE_DELETE_OPERATOR
//!		Same as MT_DECLARE_INLINE_NEW_OPERATOR, for the delete operator.
//!
#if MEMORY_CHECK == 0
#	define MT_DECLARE_INLINE_NEW_OPERATOR						\
		inline static void * operator new (size_t bytes);
#	define MT_DECLARE_INLINE_DELETE_OPERATOR					\
		inline static void operator delete (void * pointer);
#else
#	define MT_DECLARE_INLINE_NEW_OPERATOR														\
		inline static void * operator new (size_t bytes, const char * filename, int line);
#	define MT_DECLARE_INLINE_DELETE_OPERATOR													\
		inline static void operator delete (void * pointer, const char * filename, int line);	\
		inline static void operator delete (void * pointer);
#endif

//!
//! @def MT_DEFINE_INLINE_NEW_OPERATOR
//!		Define the operator new for class @a classname. See #MT_DECLARE_INLINE_NEW_OPERATOR.
//!
//! @def MT_DEFINE_INLINE_DELETE_OPERATOR
//!		Same as MT_DEFINE_INLINE_NEW_OPERATOR, for the delete operator.
//!
#if MEMORY_CHECK == 0
#	define MT_DEFINE_INLINE_NEW_OPERATOR(classname)				\
		inline void * classname::operator new (size_t bytes)
#	define MT_DEFINE_INLINE_DELETE_OPERATOR(classname)			\
		inline void classname::operator delete (void * pointer)
#else
#	define MT_DEFINE_INLINE_NEW_OPERATOR(classname)													\
		inline void * classname::operator new (size_t bytes, const char * filename, int line)
#	define MT_DEFINE_INLINE_DELETE_OPERATOR(classname)												\
		inline void classname::operator delete (void * pointer, const char *, int)					\
		{																							\
			classname::operator delete (pointer);													\
		}																							\
		inline void classname::operator delete (void * pointer)
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
	MemoryTracker::Instance().Untrack(pointer);
	free(pointer);
}

//!
//! Override the delete array operator
//!
void operator delete [] (void * pointer) noexcept
{
	MemoryTracker::Instance().Untrack(pointer);
	free(pointer);
}

#endif
