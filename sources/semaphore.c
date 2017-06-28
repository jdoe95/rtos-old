/**************************************************************************
 * SEMAPHORE FUNCTIONS FILE
 *
 * AUTHOR BUYI YU
 * 	1917804074@qq.com
 *
 * (C) 2017
 *
 * 	You should have received an open source user license.
 * 	ABOUT USAGE, MODIFICATION, COPYING, OR DISTRIBUTION, SEE LICENSE.
 **************************************************************************/
#include "../includes/config.h"
#include "../includes/types.h"
#include "../includes/global.h"
#include "../includes/functions.h"

osHandle_t
osSemaphoreCreate( osCounter_t initial )
{
	Semaphore_t* semaphore = memory_allocateFromHeap( sizeof(Semaphore_t), &kernelMemoryList );

	if( semaphore == NULL )
		return 0;

	semaphore->counter = initial;
	prioritizedList_init( &semaphore->threads );

	return (osHandle_t) semaphore;
}

void
osSemaphoreDelete( osHandle_t h )
{
	Semaphore_t* semaphore = (Semaphore_t*) h;

	osThreadEnterCritical();
	{
		thread_makeAllReady( & semaphore->threads );

		if( threads_ready.first->value < currentThread->priority )
		{
			thread_setNew();
			port_yield();
		}
	}
	osThreadExitCritical();

	memory_returnToHeap( semaphore, & kernelMemoryList );
}

void
osSemaphoreReset( osHandle_t h, osCounter_t initial )
{
	Thread_t* thread;
	SemaphoreWait_t* wait;
	Semaphore_t* semaphore = (Semaphore_t*) h;

	osThreadEnterCritical();
	{
		/* if there are threads waiting for the semaphore, unlock
		 * them as much as we can before the initial counter counts
		 * down to zero.
		 *
		 * first != NULL will be true until the list is empty.
		 *
		 * (initial--) means that will count from initial down to 1 which means
		 * the loop will execute 'initial' times if the other operant is always true */
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

osCounter_t
osSemaphoreGetCounter( osHandle_t h )
{
	Semaphore_t* semaphore = (Semaphore_t*) h;
	return semaphore->counter;
}

void
osSemaphorePost( osHandle_t h )
{
	Semaphore_t* semaphore = (Semaphore_t*) h;
	Thread_t* thread;
	SemaphoreWait_t* wait;

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

osBool_t
osSemaphorePeekWait( osHandle_t h )
{
	Semaphore_t* semaphore = (Semaphore_t*) h;
	return semaphore->counter != 0;
}

osBool_t
osSemaphoreWaitNonBlock( osHandle_t h )
{
	Semaphore_t* semaphore = (Semaphore_t*) h;
	osBool_t result = false;

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

osBool_t
osSemaphoreWait( osHandle_t h, osCounter_t timeout )
{
	Semaphore_t* semaphore = (Semaphore_t*) h;
	SemaphoreWait_t wait;
	osBool_t result = false;

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
