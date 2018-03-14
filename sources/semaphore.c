/** ************************************************************************
 * @file
 * @brief Semaphore implementation
 * @author John Doe (jdoe35087@gmail.com)
 * @details This file contains the implementation of the semaphore.
 **************************************************************************/
#include "../includes/config.h"
#include "../includes/types.h"
#include "../includes/global.h"
#include "../includes/functions.h"

/**
 * @brief Creates a semaphore
 * @param initial the initial value of the semaphore counter
 * @return handle to the semaphore, if the semaphore is created successfully;
 * 0, if the creation failed
 * @note contexts in which this function can be used
 * 	- Yes: an interrupt context
 * 	- Yes: main stack context before the kernel started
 * 	- Yes: thread contexts
 */
osHandle_t
osSemaphoreCreate( osCounter_t initial )
{
	Semaphore_t* semaphore;

	osThreadEnterCritical();
	semaphore = memory_allocateFromHeap( sizeof(Semaphore_t), &kernelMemoryList );
	osThreadExitCritical();

	if( semaphore == NULL )
	{
		OS_ASSERT(0);
		return 0;
	}

	semaphore->counter = initial;
	prioritizedList_init( &semaphore->threads );

	return (osHandle_t) semaphore;
}

/**
 * @brief Deletes a semaphore
 * @param h handle to the semaphore to be deleted
 * @details This function deletes a semaphore and releases the resources occupied
 * by the semaphore and the control structures. The threads blocked on the semaphore
 * prior to its deletion will be readied and the block will fail. The handle will be
 * invalid and should never be used again after calling this function.
 * @note contexts in which this function can be used
 * 	- Yes: an interrupt context
 * 	- No: main stack context before the kernel started
 * 	- Yes: thread contexts
 */
void
osSemaphoreDelete( osHandle_t h )
{
	Semaphore_t* semaphore = (Semaphore_t*) h;

	OS_ASSERT(h);

	osThreadEnterCritical();
	{
		thread_makeAllReady( & semaphore->threads );

		if( threads_ready.first->value < currentThread->priority )
		{
			thread_setNew();
			port_yield();
		}

		memory_returnToHeap( semaphore, & kernelMemoryList );
	}
	osThreadExitCritical();
}

/**
 * @brief Resets the semaphore to the initial state
 * @param h handle to the semaphore to be reset
 * @param initial the initial value of the semaphore counter after
 * reset
 * @note contexts in which this function can be used
 * 	- Yes: an interrupt context
 * 	- No: main stack context before the kernel started
 * 	- Yes: thread contexts
 */
void
osSemaphoreReset( osHandle_t h, osCounter_t initial )
{
	Thread_t* thread;
	SemaphoreWait_t* wait;
	Semaphore_t* semaphore = (Semaphore_t*) h;

	OS_ASSERT(h);

	osThreadEnterCritical();
	{
		/* if there are threads waiting for the semaphore, unlock
		 * them as much as we can before the initial counter counts
		 * down to zero.
		 *
		 * first != NULL will be true until the list is empty.
		 *
		 * (initial--) means that will count from initial down to 1 which means
		 * the loop will execute 'initial' times if the other operand is always true */
		while( (semaphore->threads.first != NULL) && (initial--) )
		{
			thread = (Thread_t*) semaphore->threads.first->container;
			wait = (SemaphoreWait_t*) thread->wait;

			wait->result = true;
			thread_makeReady( thread );
		}

		/* the value left after unblocking all the waiting threads */
		semaphore->counter = initial;

		if( threads_ready.first->value < currentThread->priority )
		{
			thread_setNew();
			port_yield();
		}
	}
	osThreadExitCritical();
}

/**
 * @brief Returns the value of the counter of the semaphore
 * @param h handle to the semaphore
 * @return value of the counter of the semaphore
 * @note contexts in which this function can be used
 * 	- Yes: an interrupt context
 * 	- Yes: main stack context before the kernel started
 * 	- Yes: thread contexts
 */
osCounter_t
osSemaphoreGetCounter( osHandle_t h )
{
	Semaphore_t* semaphore = (Semaphore_t*) h;
	osCounter_t ret;

	OS_ASSERT(h);

	osThreadEnterCritical();
	{
		ret = semaphore->counter;
	}
	osThreadExitCritical();
	return ret;
}

/**
 * @brief Increments the counter of the semaphore by one
 * @param h handle to the semaphore
 * @details This function will increment the counter of the semaphore
 * by one, and if threads are blocked for the semaphore, the thread with
 * the highest priority will be readied.
 * @note contexts in which this function can be used
 * 	- Yes: an interrupt context
 * 	- No: main stack context before the kernel started
 * 	- Yes: thread contexts
 */
void
osSemaphorePost( osHandle_t h )
{
	Semaphore_t* semaphore = (Semaphore_t*) h;
	Thread_t* thread;
	SemaphoreWait_t* wait;

	OS_ASSERT(h);

	osThreadEnterCritical();
	{
		/* check if any thread is waiting, transfer this value to the first
		 * highest priority thread.
		 * first != NULL means the list is not empty */
		if( semaphore->threads.first != NULL )
		{
			thread = (Thread_t*) semaphore->threads.first->container;
			wait = (SemaphoreWait_t*) thread->wait;

			wait->result = true;
			thread_makeReady( thread );

			if( thread->priority < currentThread->priority )
			{
				thread_setNew();
				port_yield();
			}
		}
		else
		{
			/* no other threads are blocking. */
			semaphore->counter++;
		}
	}
	osThreadExitCritical();
}

/**
 * @brief Tests if the counter of a semaphore can be decremented
 * @param h handle to the semaphore to be decremented
 * @retval true if the semaphore can be decremented
 * @retval false if the semaphore cannot be decremented
 * @details Beware that another thread can modify the semaphore as
 * soon as this function returns. In order to prevent this, a critical
 * section should be used if some actions are to be performed based
 * on the results of this function.
 * @note contexts in which this function can be used
 * 	- Yes: an interrupt context
 * 	- Yes: main stack context before the kernel started
 * 	- Yes: thread contexts
 */
osBool_t
osSemaphorePeekWait( osHandle_t h )
{
	Semaphore_t* semaphore = (Semaphore_t*) h;
	osBool_t ret;

	OS_ASSERT(h);

	osThreadEnterCritical();
	{
		ret = semaphore->counter != 0;
	}
	osThreadExitCritical();

	return ret;
}

/**
 * @brief Decrements the counter of a semaphore without blocking
 * @param h handle to the semaphore whose counter to be decremented
 * @retval true if the counter of the semaphore is decremented
 * @retval false if the counter of the semaphore is not decremented due to failure
 * @details Beware that another thread can modify the semaphore as
 * soon as this function returns. In order to prevent this, a critical section
 * should be used if some actions are to be performed based on the results of
 * this function.
 * @note contexts in which this function can be used
 * 	- Yes: an interrupt context
 * 	- Yes: main stack context before the kernel started
 * 	- Yes: thread contexts
 */
osBool_t
osSemaphoreWaitNonBlock( osHandle_t h )
{
	Semaphore_t* semaphore = (Semaphore_t*) h;
	osBool_t result = false;

	OS_ASSERT(h);

	osThreadEnterCritical();
	{
		if( semaphore->counter != 0 )
		{
			semaphore->counter--;
			result = true;
		}
	}
	osThreadExitCritical();
	return result;
}

/**
 * @brief Decrements the counter of a semaphore with blocking
 * @param h handle to the semaphore whose counter to be decremented
 * @param timeout the maximum time in ticks to wait for the the operation,
 * 0 can be used if indefinite
 * @retval true if the counter of the semaphore is decremented
 * @retval false if the counter of the semaphore is not decremented
 * @note contexts in which this function can be used
 * 	- No: an interrupt context
 * 	- No: main stack context before the kernel started
 * 	- Yes: thread contexts
 */
osBool_t
osSemaphoreWait( osHandle_t h, osCounter_t timeout )
{
	Semaphore_t* semaphore = (Semaphore_t*) h;
	SemaphoreWait_t wait;
	osBool_t result = false;

	OS_ASSERT(h);

	osThreadEnterCritical();
	{
		if( semaphore->counter != 0 )
		{
			semaphore->counter--;
			result = true;
		}
		else
		{
			wait.result = false;
			thread_blockCurrent( & semaphore->threads, timeout, & wait );
			result = wait.result;
		}
	}
	osThreadExitCritical();

	return result;
}
