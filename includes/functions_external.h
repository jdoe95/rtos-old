/***********************************************************************
 * RTOS EXPORTED FUNCTIONS DECLARATIONS
 *
 * AUTHOR BUYI YU
 *  1917804074@qq.com
 *
 * (C) 2017
 *
 * 	You should have received an open source user license.
 * 	ABOUT USAGE, MODIFICATION, COPYING, OR DISTRIBUTION, SEE LICENSE.
 *
 * 	Functions visible to the user should be declared here. Functions
 * 	invisible to the user, but visible to all kernel components should
 * 	be declared in functions.h. Functions visible to only 1 kernel
 * 	component should be declared in the source file of the corresponding
 * 	component.
 ***********************************************************************/
#ifndef H3C8CD7B5_313F_4475_AC94_2B2272451C73
#define H3C8CD7B5_313F_4475_AC94_2B2272451C73

#include "config.h"
#include "types_external.h"

#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************************************************/
void 			osInit						( void );
void 			osStart						( void );
osCounter_t 	osGetTime					( void );
/***********************************************************************************************/
osHandle_t 		osThreadCreate				( osCounter_t priority, osCode_t code, osCounter_t stackSize, const void *argument );
void 			osThreadDelete				( osHandle_t thread );
osThreadState_t osThreadGetState			( osHandle_t thread );
void 			osThreadSuspend			( osHandle_t thread );
void 			osThreadResume				( osHandle_t thread );
osCounter_t 	osThreadGetPriority		( osHandle_t thread );
void 			osThreadSetPriority		( osHandle_t thread, osCounter_t priority );
osHandle_t 		osThreadGetCurrentHandle	( void );
void 			osThreadDelay				( osCounter_t timeout );
void 			osThreadYield				( void );
void 			osThreadEnterCritical		( void );
void 			osThreadExitCritical		( void );
osCounter_t		osThreadGetCriticalNesting( void );
void 			osThreadSetCriticalNesting( osCounter_t counter );
osBool_t 		osThreadWaitTermination	( osHandle_t thread, osCounter_t timeout );
osBool_t		osThreadWaitTerminationAny( osHandle_t* thread, osCounter_t timeout );
/***********************************************************************************************/
void* 			osMemoryAllocate			( osCounter_t size );
void*			osMemoryReallocate		( void *p, osCounter_t size );
void 			osMemoryFree				( void *p );
osCounter_t 	osMemoryUsableSize		( void *p );
/***********************************************************************************************/
osHandle_t 		osQueueCreate					( osCounter_t size );
void 			osQueueDelete					( osHandle_t queue );
void 			osQueueReset					( osHandle_t queue );
osCounter_t 	osQueueGetSize					( osHandle_t queue );
osCounter_t 	osQueueGetUsedSize			( osHandle_t queue );
osCounter_t 	osQueueGetFreeSize			( osHandle_t queue );
osBool_t 		osQueuePeekSend				( osHandle_t queue, osCounter_t size );
osBool_t 		osQueueSendNonBlock			( osHandle_t queue, const void *data, osCounter_t size );
osBool_t 		osQueueSendAheadNonBlock		( osHandle_t queue, const void *data, osCounter_t size );
osBool_t 		osQueueSend					( osHandle_t queue, const void *data, osCounter_t size, osCounter_t timeout );
osBool_t 		osQueueSendAhead				( osHandle_t queue, const void *data, osCounter_t size, osCounter_t timeout );
osBool_t 		osQueuePeekReceive			( osHandle_t queue, osCounter_t size );
osBool_t 		osQueueReceiveNonBlock		( osHandle_t queue, void *data, osCounter_t size );
osBool_t 		osQueueReceiveBehindNonBlock( osHandle_t queue, void *data, osCounter_t size );
osBool_t 		osQueueReceive					( osHandle_t queue, void *data, osCounter_t size, osCounter_t timeout );
osBool_t 		osQueueReceiveBehind			( osHandle_t queue, void *data, osCounter_t size, osCounter_t timeout );
/***********************************************************************************************/
osHandle_t 		osSemaphoreCreate				( osCounter_t initial );
void 			osSemaphoreDelete				( osHandle_t semaphore );
void 			osSemaphoreReset				( osHandle_t semaphore, osCounter_t initial );
osCounter_t		osSemaphoreGetCounter			( osHandle_t semaphore );
void	 		osSemaphorePost				( osHandle_t semaphore );
osBool_t		osSemaphorePeekWait			( osHandle_t semaphore );
osBool_t 		osSemaphoreWaitNonBlock		( osHandle_t semaphore );
osBool_t 		osSemaphoreWait				( osHandle_t semaphore, osCounter_t timeout );
/***********************************************************************************************/
osHandle_t 		osMutexCreate					( void );
void 			osMutexDelete					( osHandle_t mutex );
osBool_t 		osMutexPeekLock				( osHandle_t mutex );
osBool_t 		osMutexLockNonBlock			( osHandle_t mutex );
osBool_t 		osMutexLock					( osHandle_t mutex, osCounter_t timeout );
void 			osMutexUnlock					( osHandle_t mutex );
/***********************************************************************************************/
osHandle_t 		osRecursiveMutexCreate		( void );
void 			osRecursiveMutexDelete		( osHandle_t mutex );
osBool_t 		osRecursiveMutexPeekLock		( osHandle_t mutex );
osBool_t 		osRecursiveMutexIsLocked		( osHandle_t mutex );
osBool_t 		osRecursiveMutexLockNonBlock( osHandle_t mutex );
osBool_t 		osRecursiveMutexLock			( osHandle_t mutex, osCounter_t timeout );
void 			osRecursiveMutexUnlock		( osHandle_t mutex );
/***********************************************************************************************/
osHandle_t 		osSignalCreate					( osCounter_t size );
void 			osSignalDelete					( osHandle_t signal );
osBool_t 		osSignalWait					( osHandle_t signal, const void* signalValue, osCounter_t timeout );
void 			osSignalSend					( osHandle_t signal, const void* signalValue );
osBool_t 		osSignalWaitAny				( osHandle_t signal, void* signalValue, osCounter_t timeout );
/***********************************************************************************************/
osHandle_t		osTimerCreate					( osTimerMode_t mode, osCounter_t priority, osCounter_t period, osCode_t callback );
void 			osTimerDelete					( osHandle_t timer );
void 			osTimerStart					( osHandle_t timer, void* argument );
void 			osTimerStop					( osHandle_t timer );
void 			osTimerReset					( osHandle_t timer );
void 			osTimerSetPeriod				( osHandle_t timer, osCounter_t period );
osCounter_t		osTimerGetPeriod				( osHandle_t timer );
void 			osTimerSetMode					( osHandle_t timer, osTimerMode_t mode );
osTimerMode_t	osTimerGetMode					( osHandle_t timer );
/***********************************************************************************************/

#ifdef __cplusplus
}
#endif

#endif /* H3C8CD7B5_313F_4475_AC94_2B2272451C73 */
