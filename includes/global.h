/** ***********************************************************************
 * @file
 * @brief Global Variables Declaration
 * @author John Doe (jdoe35087@gmail.com)
 * @details This file contains the declarations of global variables.
 *************************************************************************/
#ifndef HA77D99B0_84F3_4AB4_A348_8723510F837E
#define HA77D99B0_84F3_4AB4_A348_8723510F837E

#include "config.h"
#include "types.h"

/**
 * @defgroup os_internal_global Global Variables
 */

/**
 * @ingroup os_internal_global
 * @{
 */
extern Heap_t 						heap;				/**< @brief The heap */
extern MemoryList_t 				kernelMemoryList;	/**< @brief Memory allocated to the kernel. */
extern NotPrioritizedList_t			timerPriorityList;	/**< @brief  */

/**
 * @brief The system timeout list.
 * @details Threads that do not block permanently are put into this list.
 */
extern PrioritizedList_t 			threads_timed;
extern PrioritizedList_t 			threads_ready;		/**< @brief The ready list */

extern Thread_t 					idleThread;			/**< @brief The thread control block form the idle thread */
extern Thread_t *volatile 			currentThread;		/**< @brief Points to the current thread */
extern Thread_t *volatile 			nextThread;			/**< @brief Points to the next thread to be scheduled */
extern volatile osCounter_t			systemTime;			/**< @brief The system time */

/**
 * @brief The critical nesting counter.
 * @details This counter is stored per-thread and will be
 * saved and restored upon the blocking of the threads.
 */
extern volatile osCounter_t 		criticalNesting;

/**
 * @}
 */

#endif /* HA77D99B0_84F3_4AB4_A348_8723510F837E */
