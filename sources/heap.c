/** *********************************************************************
 * @file
 * @brief Heap function implementation
 * @author John Doe (jdoe35087@gmail.com)
 * @details This file contains the implementation of heap functions.
 ***********************************************************************/
#include "../includes/config.h"
#include "../includes/types.h"
#include "../includes/global.h"
#include "../includes/functions.h"

#include <string.h>

/**
 * @brief Creates a memory block from a piece of aligned memory
 * @param memory pointer to the aligned memory
 * @param size size of the memory, in bytes
 * @details The memory must have a size large enough to hold the memory
 * block header. The memory pointer and size must be aligned.
 */
MemoryBlock_t*
memory_blockCreate( void* memory, osCounter_t size )
{
	MemoryBlock_t* block = (MemoryBlock_t*)memory;

	/* makes sure the memory is aligned */
	OS_ASSERT( HEAP_IS_ALIGNED(memory) );
	OS_ASSERT( HEAP_IS_ALIGNED(size) );

	/* the size of the memory has to be larger than the block header */
	OS_ASSERT( size >= HEAP_ROUND_UP_SIZE(sizeof(MemoryBlock_t)) );

	block->prev = block;
	block->next = block;
	block->size = size;

	return block;
}

/**
 * @brief Splits a memory block
 * @param block pointer to the memory block to be split
 * @param size size in bytes at which the block should be split
 * @return pointer to the new memory block created after splitting
 */
MemoryBlock_t*
memory_blockSplit( MemoryBlock_t* block, osCounter_t size )
{
	MemoryBlock_t *newBlock;

	/* size must be aligned */
	OS_ASSERT( HEAP_IS_ALIGNED(size) );

	/* the block have to big enough to be split */
	OS_ASSERT( block->size >= HEAP_ROUND_UP_SIZE(sizeof(MemoryBlock_t)) + size );
	OS_ASSERT( size >= HEAP_ROUND_UP_SIZE(sizeof(MemoryBlock_t)) );

	/* create new memory block */
	newBlock = memory_blockCreate( (osByte_t*) block + size, block->size - size );

	/* update size of old memory block */
	block->size = size;

	return newBlock;
}

/**
 * @brief Inserts a memory block to a memory list
 * @param block pointer to the memory block to be inserted
 * @param list pointer to the memory list
 */
void
memory_blockInsertToMemoryList( MemoryBlock_t* block, MemoryList_t* list )
{
	if( list->first == NULL )
	{
		list->first = block;
		block->prev = block;
		block->next = block;
	}
	else
		/* insert as last block */
		listItemCookie_insertBefore( block, list->first );
}

/**
 * @brief Removes a memory block from a memory list
 * @param block pointer to the memory block to be removed
 * @param list pointer to the list from which the memory block is to be removed
 */
void
memory_blockRemoveFromMemoryList( MemoryBlock_t* block, MemoryList_t* list )
{
	if( block == list->first )
	{
		/* point first to another block */
		list->first = list->first->next;

		/* removing the only block */
		if( block == list->first )
			list->first = NULL;
	}

	listItemCookie_remove( block );
}

/**
 * @brief Inserts a memory block to the heap
 * @param block pointer to the memory block to be inserted to the heap
 */
void
memory_blockInsertToHeap( MemoryBlock_t* block )
{
	MemoryBlock_t* i;

	OS_ASSERT( criticalNesting );

	/* memory blocks in the heap are ordered by their start addresses */
	if( heap.first == NULL )
	{
		heap.first = block;
		heap.current = block;
		block->prev = block;
		block->next = block;
	}
	else if( block < heap.first )
	{
		/* insert as first block */
		listItemCookie_insertBefore(block, heap.first);
		heap.first = block;
	}
	else if( block > heap.first->prev )
	{
		/* insert as last block */
		listItemCookie_insertBefore(block, heap.first);
	}
	else
	{
		/* start from second block */
		i = heap.first->next;

		do
		{
			/* memory blocks are arranged by the value of their pointers */
			if( block < i )
				break;

			i = i->next;
		} while( true ); /* equivalent to while( i != heap.first ) */

		listItemCookie_insertBefore(block, i);
	}
}

/**
 * @brief Removes a memory block from the heap
 * @param block pointer to the memory block to be removed from the heap
 */
void
memory_blockRemoveFromHeap( MemoryBlock_t* block )
{
	OS_ASSERT( criticalNesting );

	if( block == block->next )
	{
		/* removing the only block */
		heap.current = NULL;
		heap.first = NULL;
	}
	else if( block == heap.first )
	{
		/* point first to another block */
		heap.first = heap.first->next;
	}
	else if( block == heap.current )
	{
		/* point current to another block */
		heap.current = heap.current->next;
	}

	listItemCookie_remove( block );
}

/**
 * @brief Merges adjacent blocks in the heap
 * @param block pointer to the memory to be merged with adjacent blocks
 * @return the pointer to the new memory block after being merged with
 * previous or next blocks.
 */
MemoryBlock_t*
memory_blockMergeInHeap( MemoryBlock_t* block )
{
	MemoryBlock_t* ret = block;

	OS_ASSERT( criticalNesting );

	/* if can be merged with next block */
	if( (osByte_t*) block + block->size == (osByte_t*) block->next )
	{
		if( block->next == heap.current )
			heap.current = block;

		if( block->next == heap.first )
			heap.first = block;

		block->size += block->next->size;

		listItemCookie_remove( block->next );
		ret = block;
	}

	/* if can be merged with previous block */
	if( (osByte_t*) block == (osByte_t*) block->prev + block->prev->size )
	{
		if( block == heap.current )
			heap.current = block->prev;

		if( block == heap.first )
			heap.first = block->prev;

		block->prev->size += block->size;

		listItemCookie_remove( block );
		ret = block->prev;
	}

	return ret;
}

/**
 * @brief Allocating a memory block from heap, splitting larger blocks if necessary
 * @param size the required size for the memory block
 * @return the pointer to the allocated memory block, if the block is allocated
 * successfully. NULL, if the block of required size can not be allocated
 */
MemoryBlock_t*
memory_getBlockFromHeap( osCounter_t size )
{
	MemoryBlock_t *i = NULL, *block = NULL;
	osCounter_t remainingSpace;

	OS_ASSERT( criticalNesting );

	/* at least one block in the heap */
	if( heap.first != NULL )
	{
		/* calculated size that will make the heap stay aligned */
		size = HEAP_ROUND_UP_SIZE(size) + HEAP_ROUND_UP_SIZE(sizeof(MemoryBlock_t));

		/* loop through the heap to find a block that is large enough, start searching
		 * from current block */
		i = heap.current;
		do
		{
			if( size <= i->size )
			{
				/* the remaining space of the block if the block can be split */
				remainingSpace = i->size - size;

				/* split if remaining space greater than minimum block size */
				if( remainingSpace >= HEAP_ROUND_UP_SIZE( sizeof(MemoryBlock_t) ) )
				{
					/* split the block and get the newly split block */
					block = memory_blockSplit( i, size );

					/* insert the new block into the heap */
					memory_blockInsertToHeap( block );

					/* set current pointer to the newly split block (next fit algorithm) */
					heap.current = block;
				}

				/* remove block from heap */
				memory_blockRemoveFromHeap( i );

				return i;
			}
			else
				i = i->next;

		} while( i != heap.current );

		/* no qualified blocks found */
		i = NULL;
	}

	return i;
}

/**
 * @brief Returns an allocated memory block back to the heap
 * @param block pointer to the memory block to be returned to the heap
 */
void
memory_returnBlockToHeap( MemoryBlock_t* block )
{
	OS_ASSERT( criticalNesting );

	memory_blockInsertToHeap( block );
	memory_blockMergeInHeap( block );
}

/**
 * @brief Allocates a piece of memory of at least a specified size and
 * put the memory block at destination
 * @param size the requested size in bytes for the memory block
 * @param destination pointer to the destination of the memory block
 * @return pointer to the internal memory of the memory block, if the memory
 * block is allocated successfully. NULL if the allocation failed.
 */
void*
memory_allocateFromHeap( osCounter_t size, MemoryList_t* destination )
{
	MemoryBlock_t* block;

	OS_ASSERT( criticalNesting );

	block = memory_getBlockFromHeap( size );
	if( block == NULL )
		return NULL;

	memory_blockInsertToMemoryList( block, destination );
	return HEAP_POINTER_FROM_BLOCK(block);
}

/**
 * @brief Returns an allocated memory block back to the heap
 * @param p pointer to the internal memory of the memory block
 * @param source the list the memory block is to be removed from
 */
void
memory_returnToHeap( void* p, MemoryList_t* source )
{
	MemoryBlock_t* block = HEAP_BLOCK_FROM_POINTER(p);

	OS_ASSERT( criticalNesting );

	memory_blockRemoveFromMemoryList( block, source );
	memory_returnBlockToHeap( block );
}

/**
 * @brief Allocates a piece of memory of at least the specified size from heap
 * @param size the requested size of the memory block
 * @return pointer to the memory if the memory is allocated successfully,
 * NULL if the allocation failed.
 */
void*
osMemoryAllocate( osCounter_t size )
{
	void* ret;

	osThreadEnterCritical();
	ret = memory_allocateFromHeap( size, & currentThread->localMemory );
	osThreadExitCritical();

	return ret;
}

/**
 * @brief Releases a piece of memory back to the heap
 * @param p pointer to the memory to be released
 */
void
osMemoryFree( void *p )
{
	/* in case releasing NULL pointer */
	OS_ASSERT( p != NULL );

	osThreadEnterCritical();
	memory_returnToHeap( p, & currentThread->localMemory );
	osThreadExitCritical();
}

/**
 * @brief Returns the actual allocated size of the memory
 * @param p pointer to the memory
 * @return the actual allocated size of the memory
 * @details When calling @ref osMemoryAllocate with a specified size, the actual
 * memory allocated might be larger than the requested size to keep the heap aligned.
 * The extra bytes can be utilized by the application. This function returns the
 * actual size of the allocated memory.
 */
osCounter_t
osMemoryUsableSize( void *p )
{
	return HEAP_BLOCK_FROM_POINTER(p)->size - HEAP_ROUND_UP_SIZE(sizeof(MemoryBlock_t));
}

/**
 * @brief Changes the size of the memory
 * @param p pointer to the memory
 * @param size new size of the memory
 * @return the new pointer after the size having been changed
 * @details This function changes the size of the memory block
 * pointed to by p to size bytes. The contents will be unchanged
 * in the range from the start of the region up to the minimum of
 * the old and new sizes. If the new size is larger than the old size,
 * the added memory will not be initialized. If p is NULL, then the call
 * is equivalent to @ref osMemoryAllocate. For all values of size, if
 * size is equal to zero, and p is not NULL, then the call is equivalent
 * to @ref osMemoryFree. Unless p is NULL, is must have been returned by
 * earlier call to @ref osMemoryAllocate or @ref osMemoryReallocate.
 */
void*
osMemoryReallocate( void *p, osCounter_t size )
{
	MemoryBlock_t *block = HEAP_BLOCK_FROM_POINTER(p);
	void *newP;

	if( p == NULL )
		return osMemoryAllocate(size);

	else if( size == 0 )
	{
		osMemoryFree(p);
		return NULL;
	}
	else if( size == block->size )
		return p;

	else if (size > block->size )
	{
		newP = osMemoryAllocate(size);
		memcpy( newP, p, block->size );
		return newP;
	}
	else /* size < block->size */
	{
		newP = osMemoryAllocate(size);
		memcpy( newP, p, size);
		return newP;
	}
}


