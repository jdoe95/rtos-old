/****************************************************************
 * MUTEX FUNCITONS FILE
 *
 * AUTHOR BUYI YU
 *  1917804074@qq.com
 *
 * (C) 2017
 *
 * 	You should have received an open source user license.
 * 	ABOUT USAGE, MODIFICATION, COPYING, OR DISTRIBUTION, SEE LICENSE.
 ****************************************************************/
#include "../includes/config.h"
#include "../includes/types.h"
#include "../includes/global.h"
#include "../includes/functions.h"

osHandle_t
osMutexCreate( void )
{
	/* allocate a new mutex control block */
	Mutex_t* mutex = memory_allocateFromHeap( sizeof(Mutex_t), &kernelMemoryList );

	if( mutex == NULL )
	{
		OS_ASSERT(0);
		return 0;
	}

	/* initialize the mutex control block */
	mutex->locked = false;
	prioritizedList_init( &mutex->threads );

	return (osHandle_t) mutex;
}

void
osMutexDelete( osHandle_t h )
{
	Mutex_t* mutex = (Mutex_t*) h;

	osThreadEnterCritical();
	{
		/* unblock all threads */
		thread_makeAllReady( & mutex->threads );

		if( threads_ready.first->value < currentThread->priority )
		{
			thread_setNew();
			port_yield();
		}
	}
	osThreadExitCritical();

	/* free the mutex control block */
	memory_returnToHeap( mutex, & kernelMemoryList );
}

osBool_t
osMutexPeekLock( osHandle_t h )
{
	Mutex_t* mutex = (Mutex_t*) h;

	osBool_t result;

	osThreadEnterCritical();
	{
		result = mutex->locked == false;
	}
	osThreadExitCritical();

	return result;
}

osBool_t
osMutexLockNonBlock( osHandle_t h )
{
	Mutex_t* mutex = (Mutex_t*) h;
	osBool_t result = false;

	osThreadEnterCritical();
	{
		if( mutex->locked == false )
		{
			mutex->locked = true;
			result = true;
		}
	}
	osThreadExitCritical();

	return result;
}

osBool_t
osMutexLock( osHandle_t h, osCounter_t timeout )
{
	osBool_t result = false;
	Mutex_t* mutex = (Mutex_t*) h;
	MutexWait_t wait;

	osThreadEnterCritical();
	{
		if( mutex->locked == false )
		{
			result = true;
			mutex->locked = true;
		}
		else
		{
			wait.result = false;
			thread_blockCurrent( &mutex->threads, timeout, & wait );

			result = wait.result;
		}
	}
	osThreadExitCritical();

	return result;
}

void
osMutexUnlock( osHandle_t h )
{
	Mutex_t* mutex = (Mutex_t*) h;
	Thread_t* thread;
	MutexWait_t* wait;

	osThreadEnterCritical();
	{
		if( mutex->locked )
		{
			/* pass the lock to the first highest priority thread if there are threads waiting */
			if( mutex->threads.first != NULL )
			{
				thread = (Thread_t*) mutex->threads.first->container;
				wait = (MutexWait_t*) thread->wait;

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
				mutex->locked = false;
			}
		}
	}
	osThreadExitCritical();
}

osHandle_t
osRecursiveMutexCreate( void )
{
	RecursiveMutex_t* mutex = memory_allocateFromHeap( sizeof(RecursiveMutex_t), &kernelMemoryList );
	if( mutex == NULL )
		return 0;

	mutex->counter = 0;
	mutex->owner = NULL;
	prioritizedList_init( &mutex->threads );

	return (osHandle_t) mutex;
}

void
osRecursiveMtuexDelete( osHandle_t h )
{
	RecursiveMutex_t* mutex = (RecursiveMutex_t*) h;

	osThreadEnterCritical();
	{
		thread_makeAllReady( & mutex->threads );

		if( threads_ready.first->value < currentThread->priority )
		{
			thread_setNew();
			port_yield();
		}
	}
	osThreadExitCritical();

	memory_returnToHeap( mutex, & kernelMemoryList );
}

osBool_t
osRecursiveMutexPeekLock( osHandle_t h )
{
	RecursiveMutex_t* mutex = (RecursiveMutex_t*) h;
	osBool_t result;

	osThreadEnterCritical();
	{
		/* non-atomic read, must be in critical section */
		result = ( ( mutex->counter == 0 ) || ( mutex->owner == currentThread ) );
	}
	osThreadExitCritical();

	return result;
}

osBool_t
osRecursiveMutexIsLocked( osHandle_t h )
{
	RecursiveMutex_t* mutex = (RecursiveMutex_t*) h;
	return mutex->counter != 0;
}

osBool_t
osRecursiveMutexLockNonBlock( osHandle_t h )
{
	RecursiveMutex_t* mutex = (RecursiveMutex_t*) h;
	osBool_t result = false;

	osThreadEnterCritical();
	{
		if( ( mutex->counter == 0 ) || ( mutex->owner = currentThread ) )
		{
			result = true;
			mutex->counter++;
			mutex->owner = currentThread;
		}
	}
	osThreadExitCritical();

	return result;
}

osBool_t
osRecursiveMutexLock( osHandle_t h, osCounter_t timeout )
{
	RecursiveMutex_t* mutex = (RecursiveMutex_t*) h;
	MutexWait_t wait;
	osBool_t result = false;

	osThreadEnterCritical();
	{
		if( ( mutex->counter == 0 ) || ( mutex->owner == currentThread ) )
		{
			mutex->counter++;
			mutex->owner = currentThread;
		}
		else
		{
			wait.result = false;
			thread_blockCurrent( &mutex->threads, timeout, & wait );
			result = wait.result;
		}
	}
	osThreadExitCritical();

	return result;
}

void
osRecursiveMutexUnlock( osHandle_t h )
{
	RecursiveMutex_t* mutex = (RecursiveMutex_t*) h;
	Thread_t* thread;
	MutexWait_t* wait;

	osThreadEnterCritical();
	{
		/* only unlocks if current thread owns the mutex */
		if( mutex->owner == currentThread )
		{
			if( mutex->counter > 1 )
				mutex->counter--;

			else if( mutex->counter == 1 )
			{
				/* if there are waiting threads, pass the lock to the first highest priority thread */
				if( mutex->threads.first != NULL )
				{
					thread = (Thread_t*) mutex->threads.first;
					wait = (MutexWait_t*) thread->wait;

					mutex->owner = thread;

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
					/* no threads are waiting, unlock the mutex */
					mutex->counter = 0;
				}
			}
			else
			{
				/* mutex counter == 0, trying to unlock an unlocked mutex, do nothing. */
			}
		}
	}
	osThreadExitCritical();
}

