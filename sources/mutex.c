/** **************************************************************
 * @file
 * @brief Mutex implementation
 * @author John Doe (jdoe35087@gmail.com)
 * @details This file defines the API functions for mutex and
 * recursive mutex.
 ****************************************************************/
#include "../includes/config.h"
#include "../includes/types.h"
#include "../includes/global.h"
#include "../includes/functions.h"

/**
 * @brief Creates a mutex.
 * @return handle to the created mutex, if mutex successfully created;
 * 	0, if mutex creation failed.
 * @note contexts in which this function can be used
 * 	- Yes: an interrupt context
 * 	- Yes: main stack context before the kernel started
 * 	- Yes: thread contexts
 */
osHandle_t
osMutexCreate( void )
{
	/* allocate a new mutex control block */
	Mutex_t* mutex;

	osThreadEnterCritical();
	mutex = memory_allocateFromHeap( sizeof(Mutex_t), &kernelMemoryList );
	osThreadExitCritical();

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

/**
 * @brief Deletes a mutex.
 * @param h handle to the mutex to be destroyed.
 * @details This function deletes a mutex and released the resources
 * occupied by the mutex and the control structures. The thread blocked
 * on the mutex prior to its deletion will be readied and the block will
 * fail. The handle will be invalid and should never be used again after
 * calling this function.
 * @note contexts in which this function can be used
 * 	- Yes: an interrupt context
 * 	- No: main stack context before the kernel started
 * 	- Yes: thread contexts
 */
void
osMutexDelete( osHandle_t h )
{
	Mutex_t* mutex = (Mutex_t*) h;

	OS_ASSERT(h);

	osThreadEnterCritical();
	{
		/* unblock all threads */
		thread_makeAllReady( & mutex->threads );

		if( threads_ready.first->value < currentThread->priority )
		{
			thread_setNew();
			port_yield();
		}

		/* free the mutex control block */
		memory_returnToHeap( mutex, & kernelMemoryList );
	}
	osThreadExitCritical();
}

/**
 * @brief Tests if a mutex can be locked.
 * @param h handle to the mutex to be tested.
 * @retval true if the mutex can be locked
 * @retval false if the mutex cannot be locked
 * @details
 * The mutex can be locked only when it is unlocked.
 *
 * If some actions are to be performed based on the result
 * of this function, beware that another thread can modify the
 * mutex as soon as this function returns. In order to prevent
 * this, a critical section should be used.
 * @note contexts in which this function can be used
 * 	- Yes: an interrupt context
 * 	- Yes: main stack context before the kernel started
 * 	- Yes: thread contexts
 */
osBool_t
osMutexPeekLock( osHandle_t h )
{
	Mutex_t* mutex = (Mutex_t*) h;

	OS_ASSERT(h);

	osBool_t result;

	osThreadEnterCritical();
	{
		result = mutex->locked == false;
	}
	osThreadExitCritical();

	return result;
}

/**
 * @brief Locks a mutex without blocking.
 * @param h handle to the mutex to be locked.
 * @retval true if the mutex is locked
 * @retval false if the mutex cannot be locked
 * @note contexts in which this function can be used
 * 	- Yes: an interrupt context
 * 	- Yes: main stack context before the kernel started
 * 	- Yes: thread contexts
 */
osBool_t
osMutexLockNonBlock( osHandle_t h )
{
	Mutex_t* mutex = (Mutex_t*) h;
	osBool_t result = false;

	OS_ASSERT(h);

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

/**
 * @brief Locks a mutex
 * @param h handle to the mutex to be locked
 * @param timeout maximum time to wait for the mutex to be locked, 0 for indefinite
 * @retval true if the mutex is locked
 * @retval false if timeout or failed
 * @details
 * This function will block the current thread forever or for a specified amount
 * of time to lock the mutex if it cannot be locked initially. This function cannot
 * be used in an interrupt context, see @ref osMutexLockNonBlock .
 * @note contexts in which this function can be used
 * 	- No: an interrupt context
 * 	- No: main stack context before the kernel started
 * 	- Yes: thread contexts
 */
osBool_t
osMutexLock( osHandle_t h, osCounter_t timeout )
{
	osBool_t result = false;
	Mutex_t* mutex = (Mutex_t*) h;
	MutexWait_t wait;

	OS_ASSERT(h);

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

/**
 * @brief Unlocks a mutex.
 * @param h handle to the mutex to be unlocked
 * @note contexts in which this function can be used
 * 	- Yes: an interrupt context
 * 	- No: main stack context before the kernel started
 * 	- Yes: thread contexts
 */
void
osMutexUnlock( osHandle_t h )
{
	Mutex_t* mutex = (Mutex_t*) h;
	Thread_t* thread;
	MutexWait_t* wait;

	OS_ASSERT(h);

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

/**
 * @brief Creates a recursive mutex.
 * @return handle to the mutex, if the mutex is created successfully;
 * 	0, if the creation failed.
 * @note contexts in which this function can be used
 * 	- Yes: an interrupt context
 * 	- Yes: main stack context before the kernel started
 * 	- Yes: thread contexts
 */
osHandle_t
osRecursiveMutexCreate( void )
{
	RecursiveMutex_t* mutex;

	osThreadEnterCritical();
	mutex = memory_allocateFromHeap( sizeof(RecursiveMutex_t), &kernelMemoryList );
	osThreadExitCritical();

	if( mutex == NULL )
		return 0;

	mutex->counter = 0;
	mutex->owner = NULL;
	prioritizedList_init( &mutex->threads );

	return (osHandle_t) mutex;
}

/**
 * @brief Deletes a recursive mutex.
 * @param h handle to the recursive mutex to be deleted.
 * @details This function deletes a recursive mutex and releases the
 * resources occupied by the mutex and the control structures. The threads
 * blocked on the mutex prior to its deletion will be readied and the block
 * will fail. The handle will be invalid and should never be used again
 * after calling this function.
 * @note contexts in which this function can be used
 * 	- Yes: an interrupt context
 * 	- No: main stack context before the kernel started
 * 	- Yes: thread contexts
 */
void
osRecursiveMtuexDelete( osHandle_t h )
{
	RecursiveMutex_t* mutex = (RecursiveMutex_t*) h;

	OS_ASSERT(h);

	osThreadEnterCritical();
	{
		thread_makeAllReady( & mutex->threads );

		if( threads_ready.first->value < currentThread->priority )
		{
			thread_setNew();
			port_yield();
		}

		memory_returnToHeap( mutex, & kernelMemoryList );
	}
	osThreadExitCritical();

}

/**
 * @brief Tests if a recursive mutex can be locked.
 * @param h handle to the recursive mutex to be tested.
 * @retval true if the recursive mutex can be locked
 * @retval false if the recursive mutex cannot be locked
 * @details
 * The recursive mutex can be locked when it is unlocked or when the calling thread
 * owns the recursive mutex.
 *
 * If some actions are to be performed based on the result
 * of this function, beware that another thread can modify the recursive
 * mutex as soon as this function returns. In order to prevent
 * this, a critical section should be used.
 *
 * @note contexts in which this function can be used
 * 	- Yes: an interrupt context
 * 	- Yes: main stack context before the kernel started
 * 	- Yes: thread contexts
 */
osBool_t
osRecursiveMutexPeekLock( osHandle_t h )
{
	RecursiveMutex_t* mutex = (RecursiveMutex_t*) h;
	osBool_t result;

	OS_ASSERT(h);

	osThreadEnterCritical();
	{
		/* non-atomic read, must be in critical section */
		result = ( ( mutex->counter == 0 ) || ( mutex->owner == currentThread ) );
	}
	osThreadExitCritical();

	return result;
}

/**
 * @brief Tests if a recursive mutex is locked.
 * @param h handle to the recursive mutex to be tested.
 * @retval true if the recursive mutex is locked
 * @retval false if the recursive mutex is not locked
 * @details
 * If some actions are to be performed based on the result
 * of this function, beware that another thread can modify the
 * recursive mutex as soon as this function returns. In order to prevent
 * this, a critical section should be used.
 * @note contexts in which this function can be used
 * 	- Yes: an interrupt context
 * 	- Yes: main stack context before the kernel started
 * 	- Yes: thread contexts
 */
osBool_t
osRecursiveMutexIsLocked( osHandle_t h )
{
	RecursiveMutex_t* mutex = (RecursiveMutex_t*) h;
	OS_ASSERT(h);

	osBool_t result;

	osThreadEnterCritical();
	{
		result = mutex->counter != 0;
	}
	osThreadExitCritical();

	return result;
}

/**
 * @brief Locks a recursive mutex without blocking.
 * @param h handle to the recursive mutex to be locked.
 * @retval true if the recursive mutex is locked
 * @retval false if the recursive mutex cannot be locked
 * @note contexts in which this function can be used
 * 	- Yes: an interrupt context
 * 	- Yes: main stack context before the kernel started
 * 	- Yes: thread contexts
 */
osBool_t
osRecursiveMutexLockNonBlock( osHandle_t h )
{
	RecursiveMutex_t* mutex = (RecursiveMutex_t*) h;
	osBool_t result = false;

	OS_ASSERT(h);

	osThreadEnterCritical();
	{
		if( ( mutex->counter == 0 ) || ( mutex->owner = currentThread ) )
		{
			mutex->counter++;
			mutex->owner = currentThread;
			result = true;
		}
	}
	osThreadExitCritical();

	return result;
}

/**
 * @brief Locks a recursive mutex
 * @param h handle to the recursive mutex to be locked
 * @param timeout maximum time to wait for the recursive mutex to be locked, 0 for indefinite
 * @retval true if the recursive mutex is locked
 * @retval false if timeout or failed
 * @note contexts in which this function can be used
 * 	- No: an interrupt context
 * 	- No: main stack context before the kernel started
 * 	- Yes: thread contexts
 */
osBool_t
osRecursiveMutexLock( osHandle_t h, osCounter_t timeout )
{
	RecursiveMutex_t* mutex = (RecursiveMutex_t*) h;
	MutexWait_t wait;
	osBool_t result = false;

	OS_ASSERT(h);

	osThreadEnterCritical();
	{
		if( ( mutex->counter == 0 ) || ( mutex->owner == currentThread ) )
		{
			mutex->counter++;
			mutex->owner = currentThread;
			result = true;
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

/**
 * @brief Unlocks a recursive mutex.
 * @param h handle to the recursive mutex to be unlocked
 * @note contexts in which this function can be used
 * 	- Yes: an interrupt context
 * 	- No: main stack context before the kernel started
 * 	- Yes: thread contexts
 */
void
osRecursiveMutexUnlock( osHandle_t h )
{
	RecursiveMutex_t* mutex = (RecursiveMutex_t*) h;
	Thread_t* thread;
	MutexWait_t* wait;

	OS_ASSERT(h);

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
					thread = (Thread_t*) mutex->threads.first->container;
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

