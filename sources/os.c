/************************************************************
 * RTOS SUPPORT FUNCTIONS FILE
 *
 * AUTHOR BUYI YU
 * 	1917804074@qq.com
 *
 * (C) 2017
 *
 * 	You should have received an open source user license.
 * 	ABOUT USAGE, MODIFICATION, COPYING, OR DISTRIBUTION, SEE LICENSE.
 ************************************************************/
#include "../includes/config.h"
#include "../includes/types.h"
#include "../includes/global.h"
#include "../includes/functions.h"

static osByte_t idleThreadStack[IDLE_THREAD_STACK_SIZE];

void
osInit( void )
{
	systemTime = 0;
	criticalNesting = 0;

	/* some init funcitons have to be called in a critical
	 * section. This can also prevent osThreadCreate from
	 * evoking the context switcher before osStart() */
	osThreadEnterCritical();
	{

		/* heap */
		memory_heapInit();
		port_heapInit();

		memory_listInit( & kernelMemoryList );
		prioritizedList_init( & threads_timed );
		prioritizedList_init( & threads_ready );
		notPrioritizedList_init( & timerPriorityList );

		/* create the idle thread */
		thread_init( &idleThread );
		idleThread.stackMemory = idleThreadStack;
		idleThread.PSP = port_makeFakeContext( idleThreadStack, IDLE_THREAD_STACK_SIZE, port_idle, 0 );
		idleThread.priority = OS_PRIO_LOWEST;
		idleThread.state = OSTHREAD_READY;

		/* add the idle thread to the ready list */
		prioritizedList_insert( (PrioritizedListItem_t*) ( &idleThread.schedulerListItem ), &threads_ready );

		currentThread = &idleThread;
		nextThread = &idleThread;

		/* thread termination signal */
		terminationSignal = osSignalCreate(sizeof(osHandle_t));
		OS_ASSERT( terminationSignal );

	}
	osThreadExitCritical();
}

void
osStart( void )
{
	/* set currentThread to be the first readied highest priority thread */
	currentThread = (Thread_t*) ( threads_ready.first->container );
	port_startKernel();
}

INTERRUPT void
OS_TICK_HANDLER_NAME( void )
{
	Thread_t* thread;
	PrioritizedListItem_t* listItem;

	osThreadEnterCritical();
	{
		/* increment system time */
		systemTime++;

		/* check if any thread timed out, loop will execute until
		 * being broken or list empty */
		while( threads_timed.first != NULL )
		{
			/* get the first thread and its timer list item */
			listItem = threads_timed.first;
			thread = (Thread_t*) listItem->container;

			if( systemTime >= listItem->value )
			{
				thread_makeReady( thread );
			}
			else
				/* items will be larger from now on. It's not necessary to check. Break the loop here */
				break;
		}

		/* next thread have to be pointing into the ready list */
		OS_ASSERT( nextThread->schedulerListItem.list == (void*)(&threads_ready) );

		/* Schedule a new thread every tick interrupt, move nextThread to the next item, and
		 * call the scheduler function to check if it should be run next.  */
		nextThread = (Thread_t*)( nextThread->schedulerListItem.next->container );

		thread_setNew();

		if( currentThread != nextThread )
		{
			port_yield();
		}
	}
	osThreadExitCritical();
}

osCounter_t
osGetTime( void )
{
	return systemTime;
}
