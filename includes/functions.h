/*************************************************************************
 * RTOS INTERNAL FUNCTION DECLARATION
 *
 * AUTHOR BUYI YU
 *  1917804074@qq.com
 *
 * (C) 2017
 *
 * 	You should have received an open source user license.
 * 	ABOUT USAGE, MODIFICATION, COPYING, OR DISTRIBUTION, SEE LICENSE.
 *************************************************************************/
#ifndef HD8FEBD31_8511_4019_860C_C3532E53EBF0
#define HD8FEBD31_8511_4019_860C_C3532E53EBF0

#include "types.h"
#include "global.h"
#include "functions_external.h"

/* marks a non-reentrant function (a function that accesses global resources) */
#define NREENT

/**************************************************************************
 * LIST
 **************************************************************************/
void listItemCookie_insertBefore	( ListItemCookie_t* cookie, ListItemCookie_t* position );
void listItemCookie_insertAfter		( ListItemCookie_t* cookie, ListItemCookie_t* position );
void listItemCookie_remove			( ListItemCookie_t* cookie );

INLINE void notPrioritizedList_init	( NotPrioritizedList_t* list );
INLINE void prioritizedList_init	( PrioritizedList_t* list );

void notPrioritizedList_itemInit	( NotPrioritizedListItem_t* item, void *container );
void prioritizedList_itemInit		( PrioritizedListItem_t* item, void *container, osCounter_t value );

INLINE void notPrioritizedList_insertBeforeByCookie	( NotPrioritizedListItem_t* item, NotPrioritizedListItem_t* position );
INLINE void notPrioritizedList_insertAfterByCookie	( NotPrioritizedListItem_t* item, NotPrioritizedListItem_t* position );
INLINE void notPrioritizedList_removeByCookie		( NotPrioritizedListItem_t* item );
INLINE void prioritizedList_insertBeforeByCookie	( PrioritizedListItem_t* item, PrioritizedListItem_t* position );
INLINE void prioritizedList_insertAfterByCookie		( PrioritizedListItem_t* item, PrioritizedListItem_t* position );
INLINE void prioritizedList_removeByCookie			( PrioritizedListItem_t* item );

void notPrioritizedList_insert		( NotPrioritizedListItem_t* item, NotPrioritizedList_t* list );
void notPrioritizedList_remove		( NotPrioritizedListItem_t* item );
void prioritizedList_insert			( PrioritizedListItem_t* item, PrioritizedList_t* list );
void prioritizedList_remove			( PrioritizedListItem_t* item );

NotPrioritizedListItem_t* notPrioritizedList_findContainer( const void* container, NotPrioritizedList_t* list );
PrioritizedListItem_t* prioritizedList_findContainer( const void* container, PrioritizedList_t* list );

#include "inline_functions/list.h"

/**************************************************************************
 * HEAP
 **************************************************************************/
void memory_blockInit( MemoryBlock_t* block, osCounter_t size );
INLINE void memory_listInit( MemoryList_t* list );
INLINE void memory_heapInit( void );

INLINE void memory_blockInsertBeforeByCookie	( MemoryBlock_t* block, MemoryBlock_t* position );
INLINE void memory_blockInsertAfterByCookie	( MemoryBlock_t* block, MemoryBlock_t* position );
INLINE void memory_blockRemoveByCookie			( MemoryBlock_t* block );

NREENT void memory_blockInsertToMemoryList		( MemoryBlock_t* block, MemoryList_t* list );
NREENT void memory_blockRemoveFromMemoryList	( MemoryBlock_t* block, MemoryList_t* list );

NREENT void memory_blockInsertToHeap		( MemoryBlock_t* block );
NREENT void memory_blockRemoveFromHeap		( MemoryBlock_t* block );

MemoryBlock_t* memory_blockSplit					( MemoryBlock_t* block, osCounter_t size );
NREENT MemoryBlock_t* memory_blockMergeInHeap	( MemoryBlock_t* block );
NREENT MemoryBlock_t* memory_blockFindInHeap		( void* blockStartAddress );

NREENT MemoryBlock_t* memory_getBlockFromHeap( osCounter_t size );
NREENT INLINE void memory_returnBlockToHeap( MemoryBlock_t* block );

void* memory_allocateFromHeap( osCounter_t size, MemoryList_t* destination );
void memory_returnToHeap( void* p, MemoryList_t* source );

INLINE void* memory_getPointerFromBlock( MemoryBlock_t* block );
INLINE MemoryBlock_t* memory_getBlockFromPointer( void *p );

INLINE osCounter_t memory_roundUpBlockSize( osCounter_t size );

#include "inline_functions/heap.h"

/**************************************************************************
 * THREAD
 **************************************************************************/
void threadReturnHook( void );
void thread_init( Thread_t* thread );
INLINE NREENT void thread_setNew( void );
NREENT void thread_makeReady( Thread_t* thread );
NREENT void thread_makeAllReady( PrioritizedList_t* list );
void thread_blockCurrent( PrioritizedList_t* list, osCounter_t timeout, void* wait );

#include "inline_functions/thread.h"

/**************************************************************************
 * QUEUE
 **************************************************************************/
NREENT void queue_solveEquation( osHandle_t queue );
NREENT void queue_read( Queue_t* queue, void* data, osCounter_t size );
NREENT void queue_readBehind( Queue_t* queue, void* data, osCounter_t size );
NREENT void queue_write( Queue_t* queue, const void* data, osCounter_t size );
NREENT void queue_writeAhead( Queue_t* queue, const void* data, osCounter_t size );

/**************************************************************************
 * TIMER
 **************************************************************************/
void timer_init( Timer_t* timer, osTimerMode_t mode, osCounter_t period, osCode_t callback,
	TimerPriorityBlock_t* priorityBlock );
NREENT TimerPriorityBlock_t* timer_createPriority( osCounter_t priority );
NREENT TimerPriorityBlock_t* timer_searchPriority( osCounter_t priority );
void timerTask( TimerPriorityBlock_t* volatile priorityBlock );

/**************************************************************************
 * PORT
 **************************************************************************/
void port_enableInterrupts( void );
void port_disableInterrupts( void );

/* Code that the idle thread run. Does nothing. */
void port_idle( void );

/* enables the heart tick timer and loads the first thread */
void port_startKernel( void );

/* saves current thread and loads a new thread */
void port_yield( void );

/* adds memory regions to the heap during os initialization */
void port_heapInit( void );

/* generates a fake context for newly created threads. */
osByte_t* port_makeFakeContext(
		osByte_t* stack,
		osCounter_t stackSize,
		osCode_t code,
		const void* argument );

#endif /* HD8FEBD31_8511_4019_860C_C3532E53EBF0 */
