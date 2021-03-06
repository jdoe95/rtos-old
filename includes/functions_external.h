/** *********************************************************************
 * @file
 * @brief Exported functions
 * @author John Doe (jdoe35087@gmail.com)
 * @details This file contains the API functions.
 ***********************************************************************/
#ifndef H3C8CD7B5_313F_4475_AC94_2B2272451C73
#define H3C8CD7B5_313F_4475_AC94_2B2272451C73

#include "config.h"
#include "types_external.h"

#ifdef __cplusplus
extern "C" {
#endif

/** ************************************************************************************************
 * @defgroup os_control Operating System Control and Status
 * @ingroup os_api
 * @brief Controlling the operating system and obtaining the status.
 */
/**
 * @ingroup os_control
 * @{
 */
void 			osInit						( void );
void 			osStart						( void );
osCounter_t 	osGetTime					( void );
/** @} *********************************************************************************************/
/** ************************************************************************************************
 * @defgroup os_thread Thread
 * @ingroup os_api
 * @brief Creating, deleting, controlling thread and current thread context
 */
/**
 * @ingroup os_thread
 * @{
 */
osHandle_t 		osThreadCreate				( osCounter_t priority, osCode_t code, osCounter_t stackSize, const void *argument );
void 			osThreadDelete				( osHandle_t thread );
osThreadState_t osThreadGetState			( osHandle_t thread );
void 			osThreadSuspend				( osHandle_t thread );
void 			osThreadResume				( osHandle_t thread );
osCounter_t 	osThreadGetPriority			( osHandle_t thread );
void 			osThreadSetPriority			( osHandle_t thread, osCounter_t priority );
osHandle_t 		osThreadGetCurrentHandle	( void );
void 			osThreadDelay				( osCounter_t timeout );
void 			osThreadYield				( void );
void 			osThreadEnterCritical		( void );
void 			osThreadExitCritical		( void );
osCounter_t		osThreadGetCriticalNesting	( void );
void 			osThreadSetCriticalNesting	( osCounter_t counter );
/** @} *********************************************************************************************/
/** ************************************************************************************************
 * @defgroup os_memory Dynamic Memory
 * @ingroup os_api
 * @brief Allocating and releasing dynamic memory.
 */
/**
 * @ingroup os_memory
 * @{
 */
void* 			osMemoryAllocate			( osCounter_t size );
void*			osMemoryReallocate			( void *p, osCounter_t size );
void 			osMemoryFree				( void *p );
osCounter_t 	osMemoryUsableSize			( void *p );
/** @} *********************************************************************************************/
/** ************************************************************************************************
 * @defgroup os_queue Queue
 * @ingroup os_api
 * @brief Passing large amounts of information between threads or interrupts.
 */
/**
 * @ingroup os_queue
 * @{
 */
osHandle_t 		osQueueCreate				( osCounter_t size );
void 			osQueueDelete				( osHandle_t queue );
void 			osQueueReset				( osHandle_t queue );
osCounter_t 	osQueueGetSize				( osHandle_t queue );
osCounter_t 	osQueueGetUsedSize			( osHandle_t queue );
osCounter_t 	osQueueGetFreeSize			( osHandle_t queue );
osBool_t 		osQueuePeekSend				( osHandle_t queue, osCounter_t size );
osBool_t 		osQueueSendNonBlock			( osHandle_t queue, const void *data, osCounter_t size );
osBool_t 		osQueueSend					( osHandle_t queue, const void *data, osCounter_t size, osCounter_t timeout );
osBool_t 		osQueuePeekReceive			( osHandle_t queue, osCounter_t size );
osBool_t 		osQueueReceiveNonBlock		( osHandle_t queue, void *data, osCounter_t size );
osBool_t 		osQueueReceive				( osHandle_t queue, void *data, osCounter_t size, osCounter_t timeout );
/** @} *********************************************************************************************/
/** ************************************************************************************************
 * @defgroup os_semaphore Semaphore
 * @ingroup os_api
 * @brief Synchronize threads waiting for limited resources.
 */
/**
 * @ingroup os_semaphore
 * @{
 */
osHandle_t 		osSemaphoreCreate			( osCounter_t initial );
void 			osSemaphoreDelete			( osHandle_t semaphore );
void 			osSemaphoreReset			( osHandle_t semaphore, osCounter_t initial );
osCounter_t		osSemaphoreGetCounter		( osHandle_t semaphore );
void	 		osSemaphorePost				( osHandle_t semaphore );
osBool_t		osSemaphorePeekWait			( osHandle_t semaphore );
osBool_t 		osSemaphoreWaitNonBlock		( osHandle_t semaphore );
osBool_t 		osSemaphoreWait				( osHandle_t semaphore, osCounter_t timeout );
/** @} *********************************************************************************************/
/** ************************************************************************************************
 * @defgroup os_mutex Mutex
 * @ingroup os_api
 * @brief Synchronize threads and interrupts on modification of shared resources.
 */
/**
 * @ingroup os_mutex
 * @{
 */
osHandle_t 		osMutexCreate				( void );
void 			osMutexDelete				( osHandle_t mutex );
osBool_t 		osMutexPeekLock				( osHandle_t mutex );
osBool_t 		osMutexLockNonBlock			( osHandle_t mutex );
osBool_t 		osMutexLock					( osHandle_t mutex, osCounter_t timeout );
void 			osMutexUnlock				( osHandle_t mutex );

osHandle_t 		osRecursiveMutexCreate		( void );
void 			osRecursiveMutexDelete		( osHandle_t mutex );
osBool_t 		osRecursiveMutexPeekLock	( osHandle_t mutex );
osBool_t 		osRecursiveMutexIsLocked	( osHandle_t mutex );
osBool_t 		osRecursiveMutexLockNonBlock( osHandle_t mutex );
osBool_t 		osRecursiveMutexLock		( osHandle_t mutex, osCounter_t timeout );
void 			osRecursiveMutexUnlock		( osHandle_t mutex );
/** @} *********************************************************************************************/
/** ************************************************************************************************
 * @defgroup os_signal Signal
 * @ingroup os_api
 * @brief Passing information between threads or interrupt contexts.
 */
/**
 * @ingroup os_signal
 * @{
 */
osHandle_t 		osSignalCreate				( void );
void 			osSignalDelete				( osHandle_t signal );
osBool_t 		osSignalWait				( osHandle_t signal, osSignalValue_t signalValue, void* info, osCounter_t timeout );
void 			osSignalSend				( osHandle_t signal, osSignalValue_t signalValue, const void* info, osCounter_t size );
/** @} *********************************************************************************************/
/** ************************************************************************************************
 * @defgroup os_timer Timer
 * @ingroup os_api
 * @brief Periodically executing callback functions.
 */
/**
 * @ingroup os_timer
 * @{
 */
osHandle_t		osTimerCreate				( osTimerMode_t mode, osCounter_t priority, osCounter_t period, osCode_t callback );
void 			osTimerDelete				( osHandle_t timer );
void 			osTimerStart				( osHandle_t timer, void* argument );
void 			osTimerStop					( osHandle_t timer );
void 			osTimerReset				( osHandle_t timer );
void 			osTimerSetPeriod			( osHandle_t timer, osCounter_t period );
osCounter_t		osTimerGetPeriod			( osHandle_t timer );
void 			osTimerSetMode				( osHandle_t timer, osTimerMode_t mode );
osTimerMode_t	osTimerGetMode				( osHandle_t timer );
/** @} *********************************************************************************************/

#ifdef __cplusplus
}
#endif

#endif /* H3C8CD7B5_313F_4475_AC94_2B2272451C73 */
