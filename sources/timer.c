#include "../includes/config.h"
#include "../includes/types.h"
#include "../includes/global.h"
#include "../includes/functions.h"

void
timer_initializeThreadListItem( TimerThreadListItem_t* item )
{
	item->prev = NULL;
	item->next = NULL;
	item->list = NULL;
	item->daemon = NULL;

	prioritizedList_init( & item->timerList );
	notPrioritizedList_init( & item->timerListInactive );
}

void
timer_initialize( Timer_t* timer, osTimerMode_t mode, osCode_t callback )
{
	notPrioritizedList_itemInit( &timer->timerListItem, timer );
	timer->futureTime = 0;
	timer->mode = mode;
	timer->period = 0;
	timer->callback = callback;
	timer->argument = NULL;
	timer->threadListItem = NULL;
}

void
timer_threadListInsert( TimerThreadListItem_t* item, TimerThreadList_t* list )
{
	OS_ASSERT( item->list == NULL );

	/* means that the list is empty */
	if( list->first == NULL )
	{
		list->first = item;
		item->prev = item;
		item->next = item;
	}
	else
	{
		/* insert as last item */
		listItemCookie_insertBefore( (ListItemCookie_t*)item, (ListItemCookie_t*)(list->first) );
	}

	item->list = list;
}

void
timer_threadListRemove( TimerThreadListItem_t* item )
{
	OS_ASSERT( item->list != NULL );

	if( item == item->list->first )
	{
		item->list->first = item->list->first->next;

		/* this means that the item is the only tiem in the list */
		if( item == item->list->first )
			item->list->first = NULL;
	}

	listItemCookie_remove( (ListItemCookie_t*) item );
}

TimerThreadListItem_t*
timer_createPriority( osCounter_t priority )
{
	osHandle_t thread;
	TimerThreadListItem_t* item = (TimerThreadListItem_t*)
		memory_allocateFromHeap( sizeof(TimerThreadListItem_t), & kernelMemoryList );

	if( item == NULL )
		return NULL;

	timer_initializeThreadListItem( item );

	/* create the daemon thread */
	thread = osThreadCreate( priority, timerTask, TIMER_THREAD_STACK_SIZE, item );

	if( thread == 0 )
	{
		memory_returnToHeap(item);
		return NULL;
	}

	item->daemon = (Thread_t*)thread;
	timer_threadListInsert( item, & timerThreadList );

	return item;
}

TimerThreadListItem_t*
timer_searchPriority( osCounter_t priority )
{
	TimerThreadListItem_t* item;

	if( timerThreadList.first != NULL )
	{
		item = timerThreadList.first;

		do {

			if( item->daemon->priority == priority )
				return item;

			item = item->next;
		} while( item != timerThreadList.first );
	}

	return NULL;
}

osHandle_t
osTimerCreate( osTimerMode_t mode, osCounter_t priority, osCode_t callback )
{
	Timer_t* timer = memory_allocateFromHeap( sizeof(Timer_t), & kernelMemoryList );
	TimerThreadListItem_t* threadListItem;

	if( timer == NULL )
		return 0;

	timer_initialize( timer, mode, callback );

	/* check if priority is created */
	osThreadEnterCritical();
	{
		threadListItem = timer_searchPriority(priority);

		if( threadListItem == NULL )
		{
			/* create priority */
			threadListItem = timer_createPriority( priority );

			/* failed to create priority */
			if( threadListItem == NULL )
			{
				/* free timer control block */
				memory_returnToHeap( timer );
				osThreadExitCritical();
				return 0;
			}
		}
	}
	osThreadExitCritical();

	timer->threadListItem = threadListItem;
	/* insert into inactive timer list */
	notPrioritizedList_insert( & timer->timerListItem, &threadListItem->timerListInactive );

	return (osHandle_t)timer;
}

void
osTimerDelete( osHandle_t timer )
{
	Timer_t* p = (Timer_t*) ( timer );

	osThreadEnterCritical();
	{
		if( p->timerListItem.list != NULL )
		{
			/* this will remove the list item from priorritized list or not prioritized list,
			 * whichever the item was in. */
			notPrioritizedList_remove( &p->timerListItem );
		}
	}
	osThreadExitCritical();

	memory_returnToHeap( p );
}

void
osTimerStart( osHandle_t timer, osCounter_t period, void* argument )
{
	Timer_t* p = (Timer_t*) ( timer );

	osThreadEnterCritical();
	{
		/* the timer has to be in the inactive list */
		if( p->timerListItem.list == & p->threadListItem->timerListInactive )
		{
			p->argument = argument;

			/* set the timeout period and time of first wakeup */
			p->period = period;
			p->futureTime = systemTime + period;

			/* move this timer to the active list */
			notPrioritizedList_remove( & p->timerListItem );
			prioritizedList_insert( (PrioritizedListItem_t*)& p->timerListItem, & p->threadListItem->timerList );

			/* check if the thread is alive */
			if( p->threadListItem->daemon->state == OSTHREAD_SUSPENDED )
				osThreadResume( (osHandle_t) p->threadListItem->daemon );
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
		/* the timer has to be in the active list */
		if( p->timerListItem.list == (void*) & p->threadListItem->timerList )
		{
			/* remove the timer from the active list, and put it into the inactive list */
			prioritizedList_remove( (PrioritizedListItem_t*)& p->timerListItem );
			notPrioritizedList_insert( &p->timerListItem, & p->threadListItem->timerListInactive );
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
		/* the timer has to be in the active list to be reset */
		if( p->timerListItem.list == (void*) & p->threadListItem->list )
		{
			prioritizedList_remove( (PrioritizedListItem_t*) & p->timerListItem );
			p->futureTime = systemTime + p->period;
			prioritizedList_insert( (PrioritizedListItem_t*) & p->timerListItem, & p->threadListItem->timerList );
		}
	}
	osThreadExitCritical();
}

void
osTimerSetPeriod( osHandle_t timer, osCounter_t period )
{
	Timer_t* p = (Timer_t*) ( timer );

	/* atomic operation*/
	p->period = period;
}

osCounter_t
osTimerGetPeriod( osHandle_t timer )
{
	Timer_t* p = (Timer_t*) ( timer );

	return p->period;
}

void
timerTask( TimerThreadListItem_t* item )
{
	PrioritizedListItem_t* timerItem;
	Timer_t* timer;

	osThreadEnterCritical();
	{
		for( ; ; )
		{
			while( item->timerList.first != NULL )
			{
				timerItem = item->timerList.first;

				if( systemTime >= timerItem->value )
				{
					timer = (Timer_t*) timerItem->container;

					( (TimerCallback_t) timer->callback )( timer->argument );

					prioritizedList_remove( timerItem );

					if( timer->mode == OSTIMERMODE_PERIODIC )
					{
						timerItem->value = systemTime + timer->period;
						prioritizedList_insert( timerItem, &item->timerList );
					}
					else
					{
						notPrioritizedList_insert( (NotPrioritizedListItem_t*) timerItem, & item-> timerListInactive );
					}
				}
				else
				{
					osThreadDelay( timerItem->value - systemTime );
				}
			}

			if( item->timerListInactive.first == NULL )
			{
				timer_threadListRemove( item );
				memory_returnToHeap(item);
				break;
			}
			else
			{
				osThreadSuspend(0);
			}
		} /* for(; ;); */
	}
	osThreadExitCritical();
}
