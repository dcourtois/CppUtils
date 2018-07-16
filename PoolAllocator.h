#ifndef POOL_ALLOCATOR_H
#define POOL_ALLOCATOR_H


#include "./MemoryTracker.h"


//!
//! Allocator specialized in allocating 1 type of data. It'll pre-allocate a chunk of memory
//! and use this pool to avoid allocating/deallocating too many objects. It can also be used
//! to group all objects of the same kind in contiguous memory chunks that are faster to
//! sequentially process.
//!
//! It only support allocating 1 object at a time. This is because this class is meant to be
//! used with class-defined new and delete operators. And when allocating N object, the bytes
//! count passed to the class-defined new operator might not be a multiple of the object size
//! (the implementation is allowed to add padding to store array informations) Handling this
//! is a pain in the ass, and allocating 1 at a time is good enough for my use.
//!
//! \tparam Type
//!		The type of object we'll be allocating
//!
//! \tparam count
//!		The number of objects per chunks.
//!
template< typename Type, size_t Count >
class PoolAllocator
{

	// we're using the data chunk to write the address of the next available spot, so we
	// need the size to be at least the one of a pointer.
	static_assert(
		sizeof(Type) >= sizeof(Type *),
		"PoolAllocator cannot be used with types smaller than a pointer"
	);

	//! Chained list of chunks
	struct Chunk
	{
		//! Constructor
		inline Chunk(void)
			: Previous(nullptr)
			, Data(nullptr)
			, Last(nullptr)
		{
			this->Data = reinterpret_cast< Type * >(malloc(Count * sizeof(Type)));
			memset(this->Data, 0, Count * sizeof(Type));
			this->Last = this->Data;
		}

		//! Destructor
		inline ~Chunk(void)
		{
			free(this->Data);
		}

		//! Previous chunk. nullptr for the first chunk
		Chunk * Previous;

		//! The allocated data
		Type * Data;

		//! The last chunk of this chunk
		Type * Last;
	};

public:

	//!
	//! Constructor
	//!
	PoolAllocator(void)
		: m_Chunk(nullptr)
		, m_Pointer(nullptr)
	{
	}

	//!
	//! Destructor. This'll call Clear.
	//!
	~PoolAllocator(void)
	{
		this->Clear();
	}

	//!
	//! Allocate 1 object and return a pointer to it.
	//!
	inline Type * Allocate(void)
	{
		// check if we have an empty spot
		if (m_Pointer != nullptr)
		{
			// get the address of the next free spot, and update it
			Type * pointer = m_Pointer;
			m_Pointer = *reinterpret_cast< Type ** >(m_Pointer);
			return pointer;
		}

		// check if we need to allocate a new chunk
		if (m_Chunk == nullptr || m_Chunk->Last == m_Chunk->Data + Count)
		{
			Chunk * chunk	= MT_NEW Chunk();
			chunk->Previous	= m_Chunk;
			m_Chunk			= chunk;
		}

		// return the pointer
		return m_Chunk->Last++;
	}

	//!
	//! Deallocate an object.
	//!
	inline void Deallocate(void * pointer)
	{
		// update the free spot list
		*reinterpret_cast< Type ** >(pointer) = m_Pointer;
		m_Pointer = reinterpret_cast< Type * >(pointer);
	}

	//!
	//! Clear the allocator
	//!
	void Clear(void)
	{
		while (m_Chunk != nullptr)
		{
			Chunk * previous = m_Chunk->Previous;
			MT_DELETE m_Chunk;
			m_Chunk = previous;
		}
	}

private:

	//! Current chunk
	Chunk * m_Chunk;

	//! The current pointer
	Type * m_Pointer;

};


#endif // POOL_ALLOCATOR_H
