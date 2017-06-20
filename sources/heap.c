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
	block->list = NULL;
}

void
memory_blockInsertToMemoryList( MemoryBlock_t* block, MemoryList_t* list )
{
	OS_ASSERT( block->list == NULL );

	if( list->first == NULL )
	{
		list->first = block;
		block->prev = block;
		block->next = block;
	}
	else
		/* insert as last block */
		memory_blockInsertBeforeByCookie( block, list->first );

	block->list = list;
}

void
memory_blockRemoveFromMemoryList( MemoryBlock_t* block )
{
	MemoryList_t* list = (MemoryList_t*) block->list;

	OS_ASSERT( block->list != NULL );

	if( block == list->first )
	{
		/* 'first' should always point into the list */
		list->first = list->first->next;

		if( block == list->first )
			/* this means that this item is the only item in the list */
			list->first = NULL;
	}

	memory_blockRemoveByCookie( block );
	block->list = NULL;
}

void
memory_blockInsertToHeap( MemoryBlock_t* block )
{
	MemoryBlock_t* i;

	OS_ASSERT( block->list == NULL );

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
		i = heap.first->next;

		do
		{
			if( block < i ) /* equivalent to if( (osByte_t*)(block) + block->size < (osByte_t*)i ) */
				break;

			i = i->next;
		} while( true ); /* equivalent to while( i != heap.first ) */

		memory_blockInsertBeforeByCookie( block, i );
	}
	block->list = &heap;
}

void
memory_blockRemoveFromHeap( MemoryBlock_t* block )
{
	OS_ASSERT( block->list == & heap );

	/* this function has to be called in a critical section because it accesses global resources */
	OS_ASSERT( criticalNesting );

	if( block == heap.first )
	{
		/* 'first' should always point into the heap */
		heap.first = heap.first->next;

		/* this means that the block is the only block in the heap */
		if( block == heap.first )
			heap.first = NULL;
	}

	if( block == heap.current )
	{
		/* 'current' should always point into the heap */
		heap.current = heap.current->next;

		/* this means that the block is the only block in the heap */
		if( block == heap.current )
			heap.current = NULL;
	}

	memory_blockRemoveByCookie( block );
	block->list = NULL;
}


MemoryBlock_t*
memory_blockMergeInHeap( MemoryBlock_t* block )
{
	MemoryBlock_t* ret = block;

	OS_ASSERT( block->list == &heap );

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

MemoryBlock_t*
memory_blockFindInHeap( MemoryBlock_t* block )
{
	MemoryBlock_t* i;

	/* this function has to be called in a critical section because it accesses global resources */
	OS_ASSERT( criticalNesting );

	if( heap.first != NULL )
	{
		/* heap has at least one block */
		i = heap.first;

		do {
			if( i == block )
				return block;

			i = i->next;

		} while( i != heap.first );
	}

	return NULL;
}

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

		/* start searching from current block */
		i = heap.current;

		do
		{
			if( size <= i->size )
			{
				remainingSpace = i->size - size;

				/* split if remaining space greater than minimum block size */
				if( remainingSpace >= memory_roundUpBlockSize( sizeof(MemoryBlock_t) ) ) {
					/* set current pointer to the newly splitted block, and insert it into the heap */
					block = memory_blockSplit( i, size );
					memory_blockInsertToHeap( block );
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

void
memory_returnToHeap( void* p )
{
	MemoryBlock_t* block = memory_getBlockFromPointer( p );

	osThreadEnterCritical();
	{
		memory_blockRemoveFromMemoryList( block );
		memory_returnBlockToHeap( block );
	}
	osThreadExitCritical();
}

void*
osMemoryAllocate( osCounter_t size )
{
	return memory_allocateFromHeap( size, &currentThread->localMemory );
}

void
osMemoryFree( void *p )
{
	memory_returnToHeap( p );
}

void*
osMemoryReallocate( void*p, osCounter_t newSize )
{
	void *newP;
	MemoryBlock_t *block = memory_getBlockFromPointer(p), *tempBlock;

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
	else if( targetSize == currentSize )
	{
		/* do nothing */
	}
	else if( targetSize > currentSize )
	{
		tempSize = targetSize - currentSize;

		osThreadEnterCritical();
		{
			/* check if consecutive block is free */
			tempBlock = memory_blockFindInHeap( (MemoryBlock_t*)((osByte_t*) block + block->size) );
			if( tempBlock != NULL )
			{
				if( tempBlock->size >= tempSize )
				{
					/* check if block in heap is splittable */
					if( tempBlock->size - tempSize >= memory_roundUpBlockSize(sizeof(MemoryBlock_t)) )
					{
						/* split the block  */
						memory_blockInsertToHeap(memory_blockSplit( tempBlock, tempSize ));

						/* append to original block by simply updating the size field */
						block->size = targetSize;
					}
					else
					{
						/*  append the whole block */
						memory_blockRemoveFromHeap(tempBlock);
						block->size += tempBlock->size;
					}
				}
				else
				{
					/* block found, but not enough size */
					goto copy;
				}
			}
			else
			{
				/* block not found */
copy:
				newP = osMemoryAllocate( newSize );

				if( newP != NULL )
				{
					memcpy( newP, p, currentSize );
					osMemoryFree(p);
					p = newP;
				}
				else
				{
					/* failed to allocate new memory */
					osMemoryFree(p);
					p = NULL;
				}
			}
		}
		osThreadExitCritical();
	}
	else // target size < current size
	{
		tempSize = currentSize - targetSize;

		/* check if block in memory list is splittable */
		if( tempSize >= memory_roundUpBlockSize(sizeof(MemoryBlock_t)) )
		{
			osThreadEnterCritical();
			{
				memory_returnBlockToHeap(memory_blockSplit( block, targetSize ));
			}
			osThreadExitCritical();
		}
	}

	return p;
}

osCounter_t
osMemoryUsableSize( void *p )
{
	return memory_getBlockFromPointer( p )->size - memory_roundUpBlockSize(sizeof(MemoryBlock_t));
}
