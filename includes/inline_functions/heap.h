/*********************************************************************
 * INLINED HEAP FUNCTIONS
 *
 * AUTHOR BUYI YU
 *  1917804074@qq.com
 *
 * (C) 2017
 *
 *  You should have received an open source user license.
 * 	ABOUT USAGE, MODIFICATION, COPYING, OR DISTRIBUTION, SEE LICENSE.
 *
 * 	Heap functions that would otherwise be more expensive to be defined as
 * 	not inlined.
 *********************************************************************/
#ifndef H22125E9A_D099_4545_B049_23B5E4209296
#define H22125E9A_D099_4545_B049_23B5E4209296

INLINE void
memory_listInit( MemoryList_t* list )
{
	list->first = NULL;
}

INLINE void
memory_heapInit( void )
{
	heap.first = NULL;
	heap.current = NULL;
}

INLINE void
memory_blockInsertBeforeByCookie( MemoryBlock_t* block, MemoryBlock_t* position )
{
	listItemCookie_insertBefore( (ListItemCookie_t*) block, (ListItemCookie_t*) position );
}

INLINE void
memory_blockInsertAfterByCookie( MemoryBlock_t* block, MemoryBlock_t* position )
{
	listItemCookie_insertAfter( (ListItemCookie_t*) block, (ListItemCookie_t*) position );
}

INLINE void
memory_blockRemoveByCookie( MemoryBlock_t* block )
{
	listItemCookie_remove( (ListItemCookie_t*) block );
}

INLINE osCounter_t
memory_roundUpBlockSize( osCounter_t size )
{
	if( size % MEMORY_ALIGNMENT )
		return size + ( MEMORY_ALIGNMENT - size % MEMORY_ALIGNMENT );
	else
		return size;
}

INLINE void*
memory_getPointerFromBlock( MemoryBlock_t* block )
{
	return (osByte_t*) block + memory_roundUpBlockSize( sizeof(MemoryBlock_t) );
}

INLINE MemoryBlock_t*
memory_getBlockFromPointer( void *p )
{
	return (MemoryBlock_t*) ( (osByte_t*) p - memory_roundUpBlockSize( sizeof(MemoryBlock_t) ) );
}

INLINE void
memory_returnBlockToHeap( MemoryBlock_t* block )
{
	memory_blockInsertToHeap( block );
	memory_blockMergeInHeap( block );
}


#endif /* H22125E9A_D099_4545_B049_23B5E4209296 */
