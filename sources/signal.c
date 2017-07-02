/****************************************************************
 * SIGNAL FUNCITONS FILE
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

#include <string.h>

osHandle_t
osSignalCreate( osCounter_t size )
{
	Signal_t *signal = memory_allocateFromHeap( sizeof(Signal_t), & kernelMemoryList );
	if( signal == NULL )
	{
		OS_ASSERT(0);
		return 0;
	}

	prioritizedList_init( &signal->threadsOnSignal );
	prioritizedList_init( &signal->threadsOnAnySignal );
	signal->signalSize = size;

	return (osHandle_t)( signal );
}

void
osSignalDelete( osHandle_t h )
{
	Signal_t* signal = (Signal_t*)(h);

	OS_ASSERT(h);

	osThreadEnterCritical();
	{
		thread_makeAllReady( & signal->threadsOnSignal );
		thread_makeAllReady( & signal->threadsOnAnySignal );

		if( threads_ready.first->value < currentThread->priority )
		{
			thread_setNew();
			port_yield();
		}
	}
	osThreadExitCritical();

	memory_returnToHeap( signal, & kernelMemoryList );
}

osBool_t
osSignalWait( osHandle_t h, const void* signalValue, osCounter_t timeout )
{
	Signal_t* signal = (Signal_t*)(h);
	SignalWait_t wait;
	osBool_t result;

	/* detect this at debug time. Blame the application. ;-) */
	OS_ASSERT(h);
	OS_ASSERT( signalValue != NULL );

	osThreadEnterCritical();
	{
		wait.signalValue = signalValue;
		wait.result = false;
		thread_blockCurrent( & signal->threadsOnSignal, timeout, & wait );
		result = wait.result;
	}
	osThreadExitCritical();

	return result;
}

osBool_t
osSignalWaitAny( osHandle_t h, void* signalValue, osCounter_t timeout )
{
	Signal_t* signal = (Signal_t*)(h);
	SignalAnyWait_t wait;
	osBool_t result;

	OS_ASSERT(h);

	/* signalValue can be NULL. which simply means waiting for any signal not
	 * caring about the signal value */

	osThreadEnterCritical();
	{
		wait.signalValue = signalValue;
		wait.result = false;
		thread_blockCurrent( & signal->threadsOnAnySignal, timeout, & wait );
		result = wait.result;
	}
	osThreadExitCritical();

	return result;
}

void
osSignalSend( osHandle_t h, const void* signalValue )
{
	Signal_t* signal = (Signal_t*)(h);

	SignalWait_t* wait;
	SignalAnyWait_t* anyWait;
	Thread_t* thread;
	PrioritizedListItem_t* i;

	/* detect this in debug. Blame the application. ;-) */
	OS_ASSERT(h);
	OS_ASSERT( signalValue != NULL );

	osThreadEnterCritical();
	{
		/* check threads waiting for any signal
		 * the loop will execute until the list is empty */
		while( signal->threadsOnAnySignal.first != NULL )
		{
			/* point to a thread */
			thread = (Thread_t*) signal->threadsOnAnySignal.first->container;
			anyWait = (SignalAnyWait_t*) thread->wait;

			/* copy signal to the buffer provided by the thread if not NULL */
			if( anyWait->signalValue != NULL )
				memcpy( anyWait->signalValue, signalValue, signal->signalSize );

			anyWait->result = true;
			thread_makeReady(thread);
		}

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
				if( ! memcmp(wait->signalValue, signalValue, sizeof(signal->signalSize) ) )
				{
					/* signal matched, point to next item before calling thread_makeReady to
					 * remove from list */
					if( i != i->next )
					{
						i = i->next;
						wait->result = true;
						thread_makeReady( thread );
					}
					else /* only item in the list */
					{
						wait->result = true;
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
