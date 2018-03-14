/** *********************************************************************
 * @file
 * @brief Heap related inline functions
 * @author John Doe (jdoe35087@gmail.com)
 * @details This file contains the implementation of heap related
 * inline functions.
 ***********************************************************************/
#ifndef H22125E9A_D099_4545_B049_23B5E4209296
#define H22125E9A_D099_4545_B049_23B5E4209296

/**
 * @brief Initializes a memory list
 * @param list pointer to the memory list to be initialized
 */
OS_INLINE void
memory_listInit( MemoryList_t* list )
{
	list->first = NULL;
}

/**
 * @brief Initializes the heap.
 * @note This function must be called before the first heap block
 * is inserted.
 */
OS_INLINE void
memory_heapInit( void )
{
	heap.first = NULL;
	heap.current = NULL;
}

#endif /* H22125E9A_D099_4545_B049_23B5E4209296 */
