#include "../includes/config.h"
#include "../includes/types.h"
#include "../includes/global.h"
#include "../includes/functions.h"

void
timer_init( Timer_t* timer, osTimerMode_t mode, osCounter_t period, osCode_t callback,
	TimerPriorityBlock_t* priorityBlock )
{
	notPrioritizedList_itemInit( &timer->timerListItem, timer );
	timer->futureTime = 0;
	timer->mode = mode;
	timer->period = period;
	timer->callback = callback;
	timer->argument = NULL;
	timer->timerPriorityBlock = priorityBlock;
}

void
timer_priorityBlockInit( TimerPriorityBlock_t* priorityBlock, Thread_t* daemon )
{
	notPrioritizedList_itemInit( & priorityBlock->timerPriorityListItem, priorityBlock );
	priorityBlock->daemon = daemon;
	prioritizedList_init( & priorityBlock->timerActiveList );
	notPrioritizedList_init( & priorityBlock->timerInactiveList );
}

TimerPriorityBlock_t*
timer_createPriority( osCounter_t priority )
{
	TimerPriorityBlock_t* block;
	osHandle_t daemon;

	/* this function has to be called in a critical section because it accesses global variables */
	OS_ASSERT( criticalNesting );

	/* allocate a new priority block */
	block = (TimerPriorityBlock_t*)
		memory_allocateFromHeap( sizeof(TimerPriorityBlock_t), & kernelMemoryList );

	/* check if allocated */
	if( block == NULL )
		return NULL;

	/* create a daemon thread */
	daemon = osThreadCreate( priority, (osCode_t) timerTask, TIMER_THREAD_STACK_SIZE, block );

	/* check if thread created */
	if( daemon == 0 )
	{
		/* failed to create thread */
		memory_returnToHeap( block, & kernelMemoryList );
		return NULL;
	}

	/* intiialize the newly allocated block */
	timer_priorityBlockInit( block, (Thread_t*)daemon );

	/* insert this priority block into the system timer priority list */
	notPrioritizedList_insert( & block->timerPriorityListItem, & timerPriorityList );

	return block;
}

TimerPriorityBlock_t*
timer_searchPriority( osCounter_t priority )
{
	NotPrioritizedListItem_t* i;
	TimerPriorityBlock_t* priorityBlock;

	/* this function has to be called in a critical section because it accesses global variables */
	OS_ASSERT( criticalNesting );

	if( timerPriorityList.first != NULL )
	{
		i = timerPriorityList.first;

		do {
			/* get the priority block associated with this list item */
			priorityBlock = (TimerPriorityBlock_t*) i->container;

			if( priorityBlock->daemon->priority == priority )
			{
				/* match found */
				return priorityBlock;
			}

			i = i->next;

		} while( i != timerPriorityList.first );
	}

	return NULL;
}

osHandle_t
osTimerCreate( osTimerMode_t mode, osCounter_t priority, osCounter_t period, osCode_t callback )
{
	TimerPriorityBlock_t* priorityBlock;

	/* allocate a new timer control block */
	Timer_t* timer = memory_allocateFromHeap( sizeof(Timer_t), & kernelMemoryList );

	/* check allocation */
	if( timer == NULL )
	{
		return 0;
	}

	/* check if has priority, if not, create priority */
	osThreadEnterCritical();
	{
		/* search for priotity */
		priorityBlock = timer_searchPriority( priority );

		if( priorityBlock == NULL )
		{
			/* create priority */
			priorityBlock = timer_createPriority( priority );
		}
	}
	osThreadExitCritical();

	/* check created priority */
	if( priorityBlock == NULL )
	{
		/* failed to create priority, free the timer control block */
		memory_returnToHeap( timer, & kernelMemoryList );
		return 0;
	}

	/* initialize the timer control block */
	timer_init( timer, mode, period, callback, priorityBlock );

	/* insert into inactive timer list to be started */
	notPrioritizedList_insert( & timer->timerListItem, & priorityBlock->timerInactiveList );

	return (osHandle_t) timer;
}

void
osTimerDelete( osHandle_t timer )
{
	Timer_t* p = (Timer_t*) ( timer );
	TimerPriorityBlock_t* priorityBlock;

	osThreadEnterCritical();
	{
		priorityBlock = p->timerPriorityBlock;

		/* timer has to be in the active or inactive list */
		OS_ASSERT( p->timerListItem.list );

		/* this will remove the list item from priorritized list or not prioritized list,
		 * whichever the item was in. */
		notPrioritizedList_remove( &p->timerListItem );
		memory_returnToHeap( p, & kernelMemoryList );

		/* if the thread was suspended, it will not delete the timer priority block,
		 * so it is necessary to check if there are still timers in the active or
		 * inactive timer list */
		if( (priorityBlock->timerActiveList.first == NULL) &&
			(priorityBlock->timerInactiveList.first == NULL) )
		{
			/*  delete the thread then remove and free the priority block */
			osThreadDelete( (osHandle_t) priorityBlock->daemon );

			/* remove */
			notPrioritizedList_remove( & priorityBlock->timerPriorityListItem );

			/* free */
			memory_returnToHeap( priorityBlock, & kernelMemoryList );
		}
	}
	osThreadExitCritical();
}

void
osTimerStart( osHandle_t timer, osCounter_t period, void* argument )
{
	Timer_t* p = (Timer_t*) ( timer );

	osThreadEnterCritical();
	{
		/* the timer has to be in the inactive list */
		if( p->timerListItem.list == (void*) & p->timerPriorityBlock->timerInactiveList )
		{
			p->argument = argument;

			/* set the timeout period and the time of first wakeup */
			p->period = period;
			p->futureTime = systemTime + period;

			/* move this timer to the active list */
			notPrioritizedList_remove( & p->timerListItem );
			prioritizedList_insert( (PrioritizedListItem_t*) & p->timerListItem, & p->timerPriorityBlock->timerActiveList );

			/* check if daemon thread is suspended */
			if( p->timerPriorityBlock->daemon->state == OSTHREAD_SUSPENDED )
			{
				/* resume daemon thread */
				osThreadResume( (osHandle_t) p->timerPriorityBlock->daemon);
			}

		}
	}
	osThreadExitCritical();
}

void
osTimerStop( osHandle_t timer )
{
	Timer_t* p = (Timer_t*) ( timer );

	osThreadEnterCritical();
	{
		/* if in active list, remove from it */
		if( p->timerListItem.list == (void*) & p->timerPriorityBlock->timerActiveList )
		{
			/* remove */
			prioritizedList_remove( (PrioritizedListItem_t*) & p->timerListItem );

			/* insert into inactive list */
			notPrioritizedList_insert( & p->timerListItem, & p->timerPriorityBlock->timerInactiveList );
		}
	}
	osThreadExitCritical();
}

void
osTimerReset( osHandle_t timer )
{
	Timer_t* p = (Timer_t*) ( timer );

	osThreadEnterCritical();
	{
		/* if in the active list, remove, modify and reinsert */
		if( p->timerListItem.list == (void*) & p->timerPriorityBlock->timerActiveList )
		{
			prioritizedList_remove( (PrioritizedListItem_t*) & p->timerListItem );
			p->futureTime = systemTime + p->period;
			prioritizedList_insert( (PrioritizedListItem_t*) & p->timerListItem, & p->timerPriorityBlock->timerActiveList );
		}
	}
	osThreadExitCritical();
}

void
osTimerSetPeriod( osHandle_t timer, osCounter_t period )
{
	Timer_t* p = (Timer_t*) ( timer );

	osThreadEnterCritical();
	{
		p->period = period;
	}
	osThreadExitCritical();
}

osCounter_t
osTimerGetPeriod( osHandle_t timer )
{
	Timer_t* p = (Timer_t*) ( timer );

	osCounter_t ret;

	osThreadEnterCritical();
	{
		ret = p->period;
	}
	osThreadExitCritical();

	return ret;
}

void
osTimerSetMode( osHandle_t timer, osTimerMode_t mode )
{
	Timer_t* p = (Timer_t*) timer;

	osThreadEnterCritical();
	{
		p->mode = mode;
	}
	osThreadExitCritical();
}

osTimerMode_t
osTimerGetMode( osHandle_t timer )
{
	Timer_t* p = (Timer_t*) timer;

	osTimerMode_t ret;

	osThreadEnterCritical();
	{
		ret = p->mode;
	}
	osThreadExitCritical();

	return ret;
}

/* the daemon task that calls the callback function when a timer in the active list timed out.
 *
 * If the timer active list is empty, while the inactive list is not, the thread will suspend
 * itself, until an active timer is added, when the thread should be resumed from suspension.
 * If both the active and inactive timer list is empty, the thread will exit after removing
 * and freeing its timer priority block from the system timer priority list.
 * */
void
timerTask( TimerPriorityBlock_t* volatile priorityBlock )
{
	PrioritizedListItem_t* timerListItem;
	Timer_t* timer;

	osThreadEnterCritical();
	{
		for( ; ; )
		{

			while( priorityBlock->timerActiveList.first != NULL )
			{
				/* get the first timer control block */
				timerListItem = priorityBlock->timerActiveList.first;

				/* check if timed out */
				if( systemTime >= timerListItem->value )
				{
					/* timed out, call the callback function */

					/* get the timer control block from the list item */
					timer = (Timer_t*) timerListItem->container;

					/* call the callback function */
					timer->callback( timer->argument );

					/* remove the timer from the active list. If it's periodic,
					 * update the future time and insert again. If it's one-shot,
					 * insert into the inactive list */
					prioritizedList_remove( timerListItem );

					if( timer->mode == OSTIMERMODE_PERIODIC )
					{
						timerListItem->value = systemTime + timer->period;

						prioritizedList_insert( timerListItem, & priorityBlock->timerActiveList );
					}
					else
					{
						notPrioritizedList_insert( & timer->timerListItem, & priorityBlock->timerInactiveList );
					}
				}
				else
				{
					/* not timed out, delay for remaining time */
					osThreadDelay( timerListItem->value - systemTime );
				}

			} /* while */

			/* will go here once the active timer list becomes empty */
			if( priorityBlock->timerInactiveList.first != NULL )
			{
				osThreadSuspend(0);
			}
			else
			{
				/* remove the prioirty block from the system timer priority list */
				notPrioritizedList_remove( & priorityBlock->timerPriorityListItem );

				/* free the memory */
				memory_returnToHeap( priorityBlock, & kernelMemoryList );

				/* break the loop, exit */
				break;
			}


		} /* for(; ;) */
	}
	osThreadExitCritical();
}
