/** *********************************************************************
 * @file
 * @brief RTOS Internal functions
 * @author John Doe (jdoe35087@gmail.com)
 * @details This file contains the declaration of RTOS internal functions.
 ***********************************************************************/
#ifndef HD8FEBD31_8511_4019_860C_C3532E53EBF0
#define HD8FEBD31_8511_4019_860C_C3532E53EBF0

#include "types.h"
#include "global.h"
#include "functions_external.h"

/**
 * @brief Marks a function as thread-unsafe (because it accesses global variables)
 * @details NREENT functions must be called inside critical sections.
 */
#define NREENT

/** ************************************************************************************************
 * @defgroup os_internal_list List
 */

/**
 * @ingroup os_internal_list
 * @{
 */
void listItemCookie_insertBefore	( void* cookie, void* position );
void listItemCookie_insertAfter		( void* cookie, void* position );
void listItemCookie_remove			( void* cookie );

OS_INLINE void notPrioritizedList_init	( NotPrioritizedList_t* list );
OS_INLINE void prioritizedList_init		( PrioritizedList_t* list );

void notPrioritizedList_itemInit	( NotPrioritizedListItem_t* item, void *container );
void prioritizedList_itemInit		( PrioritizedListItem_t* item, void *container, osCounter_t value );

void notPrioritizedList_insert		( NotPrioritizedListItem_t* item, NotPrioritizedList_t* list );
void prioritizedList_insert			( PrioritizedListItem_t* item, PrioritizedList_t* list );

void list_remove( void* item );
/** ************************************************************************************************
 * @}
 */

#include "inline_functions/list.h"

/** ************************************************************************************************
 * @defgroup os_internal_heap Heap
 */

/**
 * @ingroup os_internal_heap
 * @{
 */

/**
 * @brief Rounds up the memory size to the smallest aligned value
 * @param size the size in bytes to round up
 * @return size rounded up to the nearest aligned value
 */
#define HEAP_ROUND_UP_SIZE(size) \
	( ((size) % OS_MEMORY_ALIGNMENT) ? \
		((size) + OS_MEMORY_ALIGNMENT - (size) % OS_MEMORY_ALIGNMENT) : (size) )

/**
 * @brief Checks if the memory address or the memory block size is aligned
 * @param value a memory address or the size of a memory block
 * @retval true if the memory address or the memory block size is aligned
 * @retval false if the memory address or the memory block size is not aligned
 */
#define HEAP_IS_ALIGNED(value) \
	( ((osCounter_t)value) % OS_MEMORY_ALIGNMENT == 0 )

/**
 * @brief Returns the pointer to the internal memory in the memory block
 * @param block pointer to the memory block whose internal memory is to be returned
 * @retval the pointer to the internal memory of the memory block
 */
#define HEAP_POINTER_FROM_BLOCK(block) \
	( (osByte_t*)(block) + HEAP_ROUND_UP_SIZE(sizeof(MemoryBlock_t)) )

/**
 * @brief Calculates the pointer of the memory block from the pointer to
 * the internal memory
 * @param pointer the pointer to the internal memory of a memory block
 * @return the pointer to the memory block
 */
#define HEAP_BLOCK_FROM_POINTER(pointer) \
	( (MemoryBlock_t*) ((osByte_t*)pointer - HEAP_ROUND_UP_SIZE(sizeof(MemoryBlock_t))) )

OS_INLINE void memory_listInit( MemoryList_t* list );
OS_INLINE void memory_heapInit( void );

MemoryBlock_t* memory_blockCreate( void* memory, osCounter_t size );

void memory_blockInsertToMemoryList		( MemoryBlock_t* block, MemoryList_t* list );
void memory_blockRemoveFromMemoryList	( MemoryBlock_t* block, MemoryList_t* list );

NREENT void memory_blockInsertToHeap	( MemoryBlock_t* block );
NREENT void memory_blockRemoveFromHeap	( MemoryBlock_t* block );

MemoryBlock_t* memory_blockSplit				( MemoryBlock_t* block, osCounter_t size );
NREENT MemoryBlock_t* memory_blockMergeInHeap	( MemoryBlock_t* block );
NREENT MemoryBlock_t* memory_blockFindInHeap	( void* blockStartAddress );

NREENT MemoryBlock_t* memory_getBlockFromHeap	( osCounter_t size );
NREENT void memory_returnBlockToHeap			( MemoryBlock_t* block );

NREENT void* memory_allocateFromHeap	( osCounter_t size, MemoryList_t* destination );
NREENT void memory_returnToHeap			( void* p, MemoryList_t* source );

/** ************************************************************************************************
 * @}
 */

#include "inline_functions/heap.h"

/** ************************************************************************************************
 * @defgroup os_internal_thread Thread
 */

/**
 * @ingroup os_internal_thread
 * @{
 */
void threadReturnHook( void );
void thread_init( Thread_t* thread );
OS_INLINE NREENT void thread_setNew( void );
NREENT void thread_makeReady( Thread_t* thread );
NREENT void thread_makeAllReady( PrioritizedList_t* list );
NREENT void thread_blockCurrent( PrioritizedList_t* list, osCounter_t timeout, void* wait );

/** ************************************************************************************************
 * @}
 */
#include "inline_functions/thread.h"

/** ************************************************************************************************
 * @defgroup os_internal_queue Queue
 */

/**
 * @ingroup os_internal_queue
 * @{
 */
void queue_solveEquation( Queue_t* queue );
void queue_read( Queue_t* queue, void* data, osCounter_t size );
void queue_write( Queue_t* queue, const void* data, osCounter_t size );
osCounter_t queue_getUsedSize( Queue_t* queue );
osCounter_t queue_getFreeSize( Queue_t* queue );
/** ************************************************************************************************
 * @}
 */

/**************************************************************************
 * TIMER
 **************************************************************************/
void timer_init( Timer_t* timer, osTimerMode_t mode, osCounter_t period, osCode_t callback,
	TimerPriorityBlock_t* priorityBlock );
NREENT TimerPriorityBlock_t* timer_createPriority( osCounter_t priority );
NREENT TimerPriorityBlock_t* timer_searchPriority( osCounter_t priority );
void timerTask( TimerPriorityBlock_t* volatile priorityBlock );

/**
 * @defgroup portable_os_functions Portable Operating System Functions
 * @ingroup portable_layer
 * @brief Portable operating system functions
 * @details This module contains functions that are portable across different
 * architectures. These functions should be implemented in the portable layer.
 */

/** ***********************************************************************
 * @ingroup portable_os_functions
 * @{
 */
void port_enableInterrupts( void );
void port_disableInterrupts( void );

OS_NORETURN void port_idle( void );
void port_startKernel( void );
void port_yield( void );

osByte_t* port_makeFakeContext(	osByte_t* stack, osCounter_t stackSize,
		osCode_t code, const void* argument );
/** @} ********************************************************************/

#endif /* HD8FEBD31_8511_4019_860C_C3532E53EBF0 */
