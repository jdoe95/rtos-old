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
osSignalCreate( void )
{
	Signal_t *signal = memory_allocateFromHeap( sizeof(Signal_t), & kernelMemoryList );
	if( signal == NULL )
	{
		OS_ASSERT(0);
		return 0;
	}

	prioritizedList_init( &signal->threadsOnSignal );

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
osSignalWait( osHandle_t h, osSignalValue_t signalValue, void* info, osCounter_t timeout )
{
	Signal_t* signal = (Signal_t*)(h);
	SignalWait_t wait;
	osBool_t result;

	/* detect this at debug time. Blame the application. ;-) */
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

void
osSignalSend( osHandle_t h, osSignalValue_t signalValue, const void* info, osCounter_t size )
{
	Signal_t* signal = (Signal_t*)(h);

	SignalWait_t* wait;
	Thread_t* thread;
	PrioritizedListItem_t* i;

	/* detect this in debug. Blame the application. ;-) */
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
