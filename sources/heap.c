/*******************************************************************
 * HEAP FUNCTIONS
 *
 * AUTHOR BUYI YU
 *  1917804074@qq.com
 *
 * (C) 2017
 *
 * 	You should have received an open source user license.
 * 	ABOUT USAGE, MODIFICATION, COPYING, OR DISTRIBUTION, SEE LICENSE.
 *******************************************************************/
#include "../includes/config.h"
#include "../includes/types.h"
#include "../includes/global.h"
#include "../includes/functions.h"

#include <string.h>

void
memory_blockInit( MemoryBlock_t* block, osCounter_t size )
{
	OS_ASSERT( size >= memory_roundUpBlockSize(sizeof(MemoryBlock_t)) );

	block->prev = block;
	block->next = block;
	block->size = size;
}

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
		memory_blockInsertBeforeByCookie( block, list->first );
}

void
memory_blockRemoveFromMemoryList( MemoryBlock_t* block, MemoryList_t* list )
{
	if( block == list->first )
	{
		/* 'first' should always point into the list */
		list->first = list->first->next;

		if( block == list->first )
			/* this means that this item is the only item in the list */
			list->first = NULL;
	}

	memory_blockRemoveByCookie( block );
}

void
memory_blockInsertToHeap( MemoryBlock_t* block )
{
	MemoryBlock_t* i;

	/* this function has to be called in a critical section because it accesses global resources */
	OS_ASSERT( criticalNesting );

	/* memory blocks in the heap are ordered by their start
	 * addresses. See type.h for details. */

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
		memory_blockInsertBeforeByCookie( block, heap.first );
		heap.first = block;
	}
	else if( block > heap.first->prev )
	{
		/* insert as last block */
		memory_blockInsertBeforeByCookie( block, heap.first );
	}
	else
	{
		/* start from second block */
		i = heap.first->next;

		do
		{
			if( block < i ) /* equivalent to if( (osByte_t*)(block) + block->size < (osByte_t*)i ) */
				break;

			i = i->next;
		} while( true ); /* equivalent to while( i != heap.first ) */

		memory_blockInsertBeforeByCookie( block, i );
	}
}

void
memory_blockRemoveFromHeap( MemoryBlock_t* block )
{
	/* this function has to be called in a critical section because it accesses global resources */
	OS_ASSERT( criticalNesting );

	if( block == block->next )
	{
		/* this means that the block is the only block in the heap */
		heap.current = NULL;
		heap.first = NULL;
	}
	else if( block == heap.first )
	{
		/* 'first' should always point into the heap */
		heap.first = heap.first->next;
	}
	else if( block == heap.current )
	{
		/* 'current' should always point into the heap */
		heap.current = heap.current->next;
	}

	memory_blockRemoveByCookie( block );
}


MemoryBlock_t*
memory_blockMergeInHeap( MemoryBlock_t* block )
{
	MemoryBlock_t* ret = block;

	/* this function has to be called in a critical section because it accesses global resources */
	OS_ASSERT( criticalNesting );

	if( (osByte_t*) block + block->size == (osByte_t*) block->next )
	{
		/* this block can be merged with next block */

		if( block->next == heap.current )
			heap.current = block;

		if( block->next == heap.first )
			heap.first = block;

		block->size += block->next->size;

		memory_blockRemoveByCookie( block->next );
		ret = block;
	}

	if( (osByte_t*) block == (osByte_t*) block->prev + block->prev->size )
	{
		/* previous block can be merged with this block */

		if( block == heap.current )
			heap.current = block->prev;

		if( block == heap.first )
			heap.first = block->prev;

		block->prev->size += block->size;

		memory_blockRemoveByCookie( block );
		ret = block->prev;
	}

	return ret;
}

/* find if a block with a specific start address exists in a heap.
 * If so, the block will be returned.
 * If not, NULL will be returned.
 */
MemoryBlock_t*
memory_blockFindInHeap( void* blockStartAddress )
{
	MemoryBlock_t* i;

	/* this function has to be called in a critical section because it accesses global resources */
	OS_ASSERT( criticalNesting );

	if( heap.first != NULL )
	{
		/* heap has at least one block */
		i = heap.first;

		do {
			if( i == blockStartAddress )
				return (MemoryBlock_t*)blockStartAddress;

			i = i->next;

		} while( i != heap.first );
	}

	return NULL;
}

/* split a block so that it can be 'size' and return the new block.
 * If the block to be splitted is in a memory list or the heap,
 * the new block will not be inserted into the memory list or heap.
 */
MemoryBlock_t*
memory_blockSplit( MemoryBlock_t* block, osCounter_t size )
{
	MemoryBlock_t* newBlock = (MemoryBlock_t*)( (osByte_t*) block + size );

	OS_ASSERT( block->size > size );
	OS_ASSERT( size >= memory_roundUpBlockSize(sizeof(MemoryBlock_t)) );
	OS_ASSERT( block->size - size >= memory_roundUpBlockSize(sizeof(MemoryBlock_t)) );

	memory_blockInit( newBlock, block->size - size );
	block->size = size;

	return newBlock;
}

/* get a block of 'size' from heap, splitting larger blocks in the heap if necessary */
MemoryBlock_t*
memory_getBlockFromHeap( osCounter_t size )
{
	MemoryBlock_t *i = NULL, *block = NULL;
	osCounter_t remainingSpace;

	/* this function has to be called in a critical section because it accesses global resources */
	OS_ASSERT( criticalNesting );

	if( heap.first != NULL )
	{
		/* there's at least one block in the heap */

		/* calculated size that will make the heap stay aligned */
		size = memory_roundUpBlockSize( size + memory_roundUpBlockSize( sizeof(MemoryBlock_t) ) );

		/* start searching from current block (next fit algorithm) */
		i = heap.current;

		do
		{
			/* loop through the heap to find a block that is large enough */

			if( size <= i->size )
			{
				/* the remaining space of the block if the block is to be splitted */
				remainingSpace = i->size - size;

				/* split if remaining space greater than minimum block size */
				if( remainingSpace >= memory_roundUpBlockSize( sizeof(MemoryBlock_t) ) )
				{
					/* split the block and get the newly splitted block */
					block = memory_blockSplit( i, size );

					/* insert the new block into the heap */
					memory_blockInsertToHeap( block );

					/* set current pointer to the newly splitted block (next fit algorithm) */
					heap.current = block;
				}

				/* remove block from heap */
				memory_blockRemoveFromHeap( i );

				return i;
			}
			else
				i = i->next;

		} while( i != heap.current );

		/* legit block not found */
		i = NULL;
	}

	return i;
}

/* allocate a piece of memory and specify the destination for the allocated block
 * return the pointer to the free memory area of the block
 */
void*
memory_allocateFromHeap( osCounter_t size, MemoryList_t* destination )
{
	MemoryBlock_t* block;

	osThreadEnterCritical();
	{
		block = memory_getBlockFromHeap( size );
	}
	osThreadExitCritical();

	if( block == NULL )
		return NULL;

	osThreadEnterCritical();
	{
		memory_blockInsertToMemoryList( block, destination );
	}
	osThreadExitCritical();

	return memory_getPointerFromBlock( block );
}

/* put a piece of memory back to the heap and specify where the memory block
 * is coming from (should be equal to 'destination' when calling memory_allocateFromHeap)
 */
void
memory_returnToHeap( void* p, MemoryList_t* source )
{
	MemoryBlock_t* block = memory_getBlockFromPointer( p );

	osThreadEnterCritical();
	{
		memory_blockRemoveFromMemoryList( block, source );
		memory_returnBlockToHeap( block );
	}
	osThreadExitCritical();
}

void*
osMemoryAllocate( osCounter_t size )
{
	return memory_allocateFromHeap( size, & currentThread->localMemory );
}

void
osMemoryFree( void *p )
{
	memory_returnToHeap( p, & currentThread->localMemory );
}

void*
osMemoryReallocate( void*p, osCounter_t newSize )
{
	void *newP;
	MemoryBlock_t *block = memory_getBlockFromPointer(p), *tempBlock;
	osBool_t copy = false;

	osCounter_t currentSize = block->size,
		targetSize = memory_roundUpBlockSize( newSize + memory_roundUpBlockSize(sizeof(MemoryBlock_t)) ),
		tempSize;

	if( p == NULL )
	{
		return osMemoryAllocate(newSize);
	}
	else if( newSize == 0 )
	{
		osMemoryFree(p);
		p = NULL;
	}
	else if( targetSize > currentSize )
	{
		/* calculate number of missing bytes */
		tempSize = targetSize - currentSize;

		osThreadEnterCritical();
		{
			/* check if a consecutive block is in the heap */
			tempBlock = memory_blockFindInHeap( (MemoryBlock_t*)((osByte_t*) block + block->size) );

			if( tempBlock != NULL )
			{
				/* found a consecutive free block */
				if( tempBlock->size >= tempSize )
				{
					/* check if block in heap is splittable */
					if( tempBlock->size - tempSize >= memory_roundUpBlockSize(sizeof(MemoryBlock_t)) )
					{
						/* split the block and insert the newly splitted block to the heap */
						memory_blockInsertToHeap(memory_blockSplit( tempBlock, tempSize ));

						/* append by simply updating the size field */
						block->size = targetSize;
					}
					else
					{
						/* remove the whole block from heap and append the whole block */
						memory_blockRemoveFromHeap(tempBlock);
						block->size += tempBlock->size;
					}
				}
				else
				{
					/* block found, but not of enough size */
					copy = true;
				}
			}
			else
			{
				/* block not found */
				copy = true;
			}

			/* allocate new memory and copy the old content */
			if( copy )
			{
				newP = osMemoryAllocate( newSize );

				if( newP != NULL )
				{
					/* copy old content */
					memcpy( newP, p, currentSize );

					/* free the old memory after copying */
					osMemoryFree(p);

					/* set return value */
					p = newP;
				}
				else
				{
					/* Failed to allocate new memory. Set the return value to NULL.
					 * The application still get to keep the old memory. */
					p = NULL;
				}
			}
		}
		osThreadExitCritical();
	}
	else /* target size < current size */
	{
		/* calculate number of redundant bytes */
		tempSize = currentSize - targetSize;

		/* check if block is splittable */
		if( tempSize >= memory_roundUpBlockSize(sizeof(MemoryBlock_t)) )
		{
			osThreadEnterCritical();
			{
				/* split the block and return the redundant new block to heap */
				memory_returnBlockToHeap(memory_blockSplit( block, targetSize ));
			}
			osThreadExitCritical();
		}
	}

	return p;
}

/* get the number of bytes that is actually allocated */
osCounter_t
osMemoryUsableSize( void *p )
{
	return memory_getBlockFromPointer( p )->size - memory_roundUpBlockSize(sizeof(MemoryBlock_t));
}
