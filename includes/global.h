/****************************************************************************
 * RTOS GLOBAL VARIABLE DEFINITIONS
 *
 * AUTHOR BUYI YU
 *  1917804074@qq.com
 *
 * (C) 2017
 *
 *  You should have received an open source user license.
 * 	ABOUT USAGE, MODIFICATION, COPYING, OR DISTRIBUTION, SEE LICENSE.
 ***************************************************************************/
#ifndef HA77D99B0_84F3_4AB4_A348_8723510F837E
#define HA77D99B0_84F3_4AB4_A348_8723510F837E

#include "config.h"
#include "types.h"

extern Heap_t 						heap;						/* Available dynamic memory in the system */
extern MemoryList_t 				kernelMemoryList;			/* Memory allocated from the heap currently in use by the kernel. */

extern TimerThreadList_t	 		timerThreadList;			/* List of timer lists. The timers are grouped by their daemon. */

extern PrioritizedList_t 			threads_timed;				/* Threads in a timed-block state */
extern PrioritizedList_t 			threads_ready;				/* Threads in ready state */

extern Thread_t 					idleThread;					/* Idle thread TCB */
extern Thread_t *volatile 			currentThread;				/* thread to which current context should be saved */
extern Thread_t *volatile 			nextThread;					/* thread from which new context shoule be loaded, if context switch is to take place */

extern volatile osCounter_t			systemTime;					/* Incremented on every tick */
extern volatile osCounter_t 		criticalNesting;			/* Temporary value, saved on every thread's context */

extern osHandle_t 					terminationSignal;

#endif /* HA77D99B0_84F3_4AB4_A348_8723510F837E */
