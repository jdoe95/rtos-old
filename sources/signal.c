/** **********************************************************
 * @file
 * @brief Signal Functions
 * @author John Doe (jdoe35087@gmail.com)
 * @details This file contains the functions for OS Signal.
 * ************************************************************/
#include "../includes/config.h"
#include "../includes/types.h"
#include "../includes/global.h"
#include "../includes/functions.h"

#include <string.h>

/**
 * @brief Creates a signal
 * @return the handle to the created signal, if the signal is
 * created successfully; 0, if the creation failed.
 * @note contexts in which this function can be used
 * 	- Yes: an interrupt context
 * 	- Yes: main stack context before the kernel started
 * 	- Yes: thread contexts
 */
osHandle_t
osSignalCreate( void )
{
	Signal_t *signal;

	osThreadEnterCritical();
	signal = memory_allocateFromHeap( sizeof(Signal_t), & kernelMemoryList );
	osThreadExitCritical();

	/* sanity check on allocation */
	if( signal == NULL )
	{
		OS_ASSERT(0);
		return 0;
	}

	prioritizedList_init( &signal->threadsOnSignal );
	return (osHandle_t)( signal );
}

/**
 * @brief Deletes a signal
 * @param h the handle to the signal to be deleted
 * @details This function will destroy the signal and release the occupied
 * resources. The threads blocked on this signal prior to its deletion
 * will be readied and the block will fail. The handle will be invalid and
 * should never be used again after calling this function.
 * @note contexts in which this function can be used
 * 	- Yes: an interrupt context
 * 	- No: main stack context before the kernel started
 * 	- Yes: thread contexts
 */
void
osSignalDelete( osHandle_t h )
{
	Signal_t* signal = (Signal_t*)(h);

	OS_ASSERT(h);

	osThreadEnterCritical();
	{
		thread_makeAllReady( & signal->threadsOnSignal );

		if( threads_ready.first->value < currentThread->priority )
		{
			thread_setNew();
			port_yield();
		}

		/* release memory */
		memory_returnToHeap( signal, & kernelMemoryList );
	}
	osThreadExitCritical();
}

/**
 * @brief Waits for a signal value on a signal
 * @param h handle to the signal on which the signal value is sent to
 * @param signalValue the signal value to wait for
 * @param info an optional pointer to a struct to store received information
 * sent by @ref osSignalSend, pass NULL for none.
 * @param timeout the maximum time in ticks to wait for the signal, 0 can
 * be used if indefinite
 * @retval true if the signal value is received during the timeout period
 * @retval false if the signal value is not received during the timeout period
 * @note contexts in which this function can be used
 * 	- No: an interrupt context
 * 	- No: main stack context before the kernel started
 * 	- Yes: thread contexts
 */
osBool_t
osSignalWait( osHandle_t h, osSignalValue_t signalValue, void* info, osCounter_t timeout )
{
	Signal_t* signal = (Signal_t*)(h);
	SignalWait_t wait;
	osBool_t result;

	OS_ASSERT(h);

	osThreadEnterCritical();
	{
		wait.info = info;
		wait.signalValue = signalValue;
		wait.result = false;
		thread_blockCurrent( & signal->threadsOnSignal, timeout, & wait );
		result = wait.result;
	}
	osThreadExitCritical();

	return result;
}

/**
 * @brief Sends a signal value onto a signal to wake up the associated threads
 * @param h the handle to the signal to which the signal value will be sent
 * @param signalValue the signal value to be sent
 * @param info an optional pointer to a structure to be sent with the signal value
 * so that it can be received by threads calling @ref osSignalWait. Pass NULL if not
 * used.
 * @param size the size of the information structure.
 * @note contexts in which this function can be used
 * 	- Yes: an interrupt context
 * 	- No: main stack context before the kernel started
 * 	- Yes: thread contexts
 */
void
osSignalSend( osHandle_t h, osSignalValue_t signalValue, const void* info, osCounter_t size )
{
	Signal_t* signal = (Signal_t*)(h);

	SignalWait_t* wait;
	Thread_t* thread;
	PrioritizedListItem_t* i;

	OS_ASSERT(h);

	osThreadEnterCritical();
	{
		/* check if there are threads waiting for this specific signal */
		i = signal->threadsOnSignal.first;
		if( i != NULL )
		{
			do
			{
				/* point to a thread */
				thread = (Thread_t*) i->container;
				wait = (SignalWait_t*) thread->wait;

				/* compare signal to the buffer provided by the thread. */
				if( wait->signalValue == signalValue )
				{
					/* signal matched, point to next item before calling thread_makeReady to
					 * remove from list */
					if( i != i->next )
					{
						i = i->next;
						wait->result = true;
						if( size > 0 )
						{
							if( (wait->info != NULL) && (info != NULL) )
								memcpy( wait->info, info, size );
						}
						thread_makeReady( thread );
					}
					else /* only item in the list */
					{
						wait->result = true;
						if( size > 0 )
						{
							if( (wait->info != NULL) && (info != NULL) )
								memcpy( wait->info, info, size );
						}
						thread_makeReady( thread );
						break;
					}
				}
				else
				{
					i = i->next;
				}

			} while( i != signal->threadsOnSignal.first );
		}

		if( threads_ready.first->value < currentThread->priority )
		{
			thread_setNew();
			port_yield();
		}
	}
	osThreadExitCritical();
}
