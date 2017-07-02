#include "../includes/config.h"
#include "../includes/types.h"
#include "../includes/global.h"
#include "../includes/functions.h"

/* this function's address should be stored in the last frame
 * of the thread's stack in order for it to be popped out to the PC
 * if the thread returns. As a result, the thread will jump here. */
void
threadReturnHook( void )
{
	/* when this function is executing, current thread should be in
	 * the ready state, because it needs to delete itself. */
	OS_ASSERT( currentThread->state == OSTHREAD_READY );

	/* Thread exited. Pass 0 to destroy current thread */
	osThreadDelete(0);
}

/* initialize the list items and lists in the thread control block. */
void
thread_init( Thread_t* thread )
{
	notPrioritizedList_itemInit( &thread->schedulerListItem, thread );
	prioritizedList_itemInit( &thread->timerListItem, thread, 0 );
	memory_listInit( &thread->localMemory );
	thread->wait = NULL;
}

/* ready a thread */
void
thread_makeReady( Thread_t* thread )
{
	/* this function has to be called in a critical section because it accesses global resources */
	OS_ASSERT( criticalNesting );

	/* the thread to be readied must not be in the ready list */
	OS_ASSERT( thread->schedulerListItem.list != (void* )&threads_ready );

	/* If the scheduler list item is in a thread list (usually some kernel object's blocking
	 * list), remove it.
	 * When calling functions like osThreadDelay, the scheduler will not be put into any list, so
	 * it's necessary to check before removing */
	if( thread->schedulerListItem.list != NULL )
	{
		/* this will remove the list item from both prioritizedList and NotPrioritizedList,
		 * whichever the item was in. */
		notPrioritizedList_remove( &thread->schedulerListItem );
	}

	/* If the thread was in a timed blocking, we also need to remove it from the system
	 * timeout list. */
	if( thread->timerListItem.list != NULL )
		prioritizedList_remove( &thread->timerListItem );

	/* remove the docked wait struct. */
	thread->wait = NULL;

	/* insert the thread into the ready list, and set its state to ready */
	prioritizedList_insert( (PrioritizedListItem_t*) ( &thread->schedulerListItem ), &threads_ready );
	thread->state = OSTHREAD_READY;
}

/* make all threads in a list ready. */
void
thread_makeAllReady( PrioritizedList_t* list )
{
	Thread_t* thread;

	/* this function has to be called in a critical section because it accesses global resources. */
	OS_ASSERT( criticalNesting );

	/* the list must not be the ready list */
	OS_ASSERT( list != & threads_ready );

	/* the loop will execute until the list is empty */
	while( list->first != NULL )
	{
		/* point to a thread in the list */
		thread = (Thread_t*) list->first->container;

		/* ready the thread, thus removing it from the list */
		thread_makeReady( thread );
	}
}

void
thread_blockCurrent( PrioritizedList_t* list, osCounter_t timeout, void* wait )
{
	/* define the critical nesting counter on the thread's stack */
	osCounter_t criticalNestingSave;

	/* this function has to be called in a critical section because it accesses global resources. */
	OS_ASSERT( criticalNesting );

	/* current thread is expected to be in the ready list */
	OS_ASSERT( currentThread->schedulerListItem.list == (void* )( &threads_ready ) );

	/* current thread is expected to be in a ready state */
	OS_ASSERT( currentThread->state == OSTHREAD_READY );

	/* make sure nextThread is always in the ready list. Current thread is about
	 * to be removed from it */
	if( currentThread == nextThread )
	{
		/* the idle thread is always in the list so it's unnecessary to check if nextThread
		 * is set to itself because in that case it will be set to nextThread */
		nextThread = (Thread_t*)( nextThread->schedulerListItem.next->container );
	}

	/* this will remove the list item from both prioritizedList and NotPrioritizedList,
	 * whichever the item was in. */
	notPrioritizedList_remove( &currentThread->schedulerListItem );
	currentThread->state = OSTHREAD_BLOCKED;

	/* insert into resource's thread blocking list, if any */
	if( list != NULL )
		prioritizedList_insert( (PrioritizedListItem_t*) &currentThread->schedulerListItem, list );

	/* a finite timeout value, add to system timer list */
	if( timeout != 0 )
	{
		/* calculate next wakeup time */
		currentThread->timerListItem.value = timeout + systemTime;

		/* Insert into timed thread list. When this thread is readied, timerListItem will be
		 * automatically removed from timed thread list. */
		prioritizedList_insert( &currentThread->timerListItem, &threads_timed );
	}

	/* dock the wait struct */
	currentThread->wait = wait;

	thread_setNew();
	OS_ASSERT( currentThread != nextThread );

	/* the critical nesting counter is stored per-thread, save criticalNesting to stack */
	criticalNestingSave = criticalNesting;
	criticalNesting = 0;

	/* open a window for the context switcher */
	port_enableInterrupts();
	{
		port_yield();
	}
	port_disableInterrupts();
	
	/* restore the critical nesting from the thread's stack */
	criticalNesting = criticalNestingSave;

	/* the thread should be in the ready list when resumed from port_yield() */
	OS_ASSERT( currentThread->schedulerListItem.list == (void* )( &threads_ready ) );

	/* the thread should be in the ready state when resumed from port_yield */
	OS_ASSERT( currentThread->state == OSTHREAD_READY );

	/* the timer list item should be removed */
	OS_ASSERT( currentThread->timerListItem.list == NULL );
}

/* kernel level mutual exclusion. Supports nesting. See type.h */
void
osThreadEnterCritical( void )
{
	/* enter critical section before accessing global resource 'criticalNesting' */
	port_disableInterrupts();
	criticalNesting++;
}

/* kernel level mutual exclusion. Supports nesting. See type.h */
void
osThreadExitCritical( void )
{
	if( criticalNesting > 1 )
		criticalNesting--;

	else if( criticalNesting == 1 )
	{
		criticalNesting = 0;
		port_enableInterrupts();
	}
	else
	{
		// does nothing
	}
}

osHandle_t
osThreadCreate( osCounter_t priority, osCode_t code, osCounter_t stackSize, const void* argument )
{
	Thread_t* thread;
	osByte_t* stackMemory;

	/* allocate a thread control block */
	thread = (Thread_t*) memory_allocateFromHeap( sizeof(Thread_t), &kernelMemoryList );
	if( thread == NULL )
	{
		OS_ASSERT(0);
		return 0;
	}

	/* allocate stack memory for the thread */
	stackMemory = (osByte_t*) memory_allocateFromHeap( stackSize, &kernelMemoryList );
	if( stackMemory == NULL )
	{
		memory_returnToHeap( thread, & kernelMemoryList );
		OS_ASSERT(0);
		return 0;
	}

	thread_init( thread );
	thread->PSP = port_makeFakeContext( stackMemory, stackSize, code, argument );
	thread->priority = priority;
	thread->stackMemory = stackMemory;

	osThreadEnterCritical();
	{
		thread_makeReady( thread );
	}
	osThreadExitCritical();
	return (osHandle_t) thread;
}

osThreadState_t
osThreadGetState( osHandle_t thread )
{
	Thread_t* p = (Thread_t*) thread;

	OS_ASSERT(thread);

	if( thread == 0 )
		p = currentThread;

	osThreadState_t ret;

	osThreadEnterCritical();
	{
		ret = p->state;
	}
	osThreadExitCritical();

	return ret;
}

void
osThreadDelete( osHandle_t thread )
{
	Thread_t* p = (Thread_t*) thread;
	MemoryBlock_t* block;

	OS_ASSERT(thread);

	if( thread == 0 )
		p = currentThread;

	/* send a signal about the thread's termination */
	osSignalSend( terminationSignal, & p );

	osThreadEnterCritical();
	{
		/* schedulerListItem might not be in any lists, so it is necessary to check */
		if( p->schedulerListItem.list != NULL )
		{
			/* makes sure nextThread is in the ready list */
			if( p == nextThread )
				nextThread = (Thread_t*)( nextThread->schedulerListItem.next->container );

			/* this will remove the list item from both prioritizedList and NotPrioritizedList,
			 * whichever the item was in. */
			notPrioritizedList_remove( &p->schedulerListItem );
		}

		/* The thread is in a timed blocking */
		if( p->timerListItem.list != NULL )
		{
			prioritizedList_remove( &p->timerListItem );
		}

		/* Free all allocated local memory */
		/* loop will execure until list is empty */
		while( p->localMemory.first != NULL )
		{
			/* point to a memory block */
			block = p->localMemory.first;

			memory_blockRemoveFromMemoryList( block, & currentThread->localMemory );
			memory_returnBlockToHeap( block );
		}

		/* after this, the thread will still try to push some data onto
		 * its stack. This, however, will not corrupt the heap.	 */
		memory_returnToHeap( p->stackMemory, & kernelMemoryList );
		memory_returnToHeap( p, & kernelMemoryList );

		/* yield if deleting current thread */
		if( p == currentThread )
		{
			thread_setNew();
			port_yield();
		}
	}
	osThreadExitCritical();
}

osBool_t osThreadWaitTermination( osHandle_t thread, osCounter_t timeout )
{
	OS_ASSERT(thread);

	return osSignalWait( terminationSignal, & thread, timeout );
}

osBool_t osThreadWaitTerminationAny( osHandle_t* thread, osCounter_t timeout )
{
	return osSignalWaitAny( terminationSignal, thread, timeout );
}

void
osThreadSuspend( osHandle_t thread )
{
	Thread_t* p = (Thread_t*) thread;

	/* define the critical nesting counter on the thread's stack */
	osCounter_t criticalNestingSave;

	OS_ASSERT(thread);

	if( thread == 0 )
		p = currentThread;

	osThreadEnterCritical();
	{
		/* add support for suspending multiple times */
		if( p->state != OSTHREAD_SUSPENDED )
		{
			/* schedulerListItem might not be in any lists, so it's necessary to check */
			if( p->schedulerListItem.list != NULL )
			{
				/* makes sure nextThread is in the ready list */
				if( p == nextThread )
					nextThread = (Thread_t*)( nextThread->schedulerListItem.next->container );

				/* this will remove the list item from both prioritizedList and NotPrioritizedList,
				 * whichever the item was in. */
				notPrioritizedList_remove( &p->schedulerListItem );
			}

			p->state = OSTHREAD_SUSPENDED;

			/* The thread is in a timed-wait */
			if( p->timerListItem.list != NULL )
			{
				prioritizedList_remove( &p->timerListItem );
			}

			/* remove docked wait struct */
			p->wait = NULL;

			/* Only needs to yield when suspending current thread */
			if( p == currentThread )
			{
				thread_setNew();
				OS_ASSERT( currentThread != nextThread );

				/* the critical nesting counter is stored per-thread, save criticalNesting to stack */
				criticalNestingSave = criticalNesting;

				/* open a window for the context switcher */
				port_enableInterrupts();
				{
					port_yield();
				}
				port_disableInterrupts();

				/* restore the critical nesting from the thread's stack */
				criticalNesting = criticalNestingSave;
			}
		} /* if not suspended */
	}
	osThreadExitCritical();
}

void
osThreadResume( osHandle_t thread )
{
	Thread_t* p = (Thread_t*) thread;

	OS_ASSERT(thread);

	osThreadEnterCritical();
	{
		/* adds support for resuming multiple times */
		if( p->state == OSTHREAD_SUSPENDED )
		{
			thread_makeReady( p );

			/* yield immediately if the newly readied thread has a higher priority */
			if( p->priority < currentThread->priority )
			{
				thread_setNew();
				port_yield();
			}
		}
	}
	osThreadExitCritical();
}

osCounter_t
osThreadGetPriority( osHandle_t thread )
{
	Thread_t* p = (Thread_t*) thread;

	OS_ASSERT(thread);

	if( thread == 0 )
		p = currentThread;

	osCounter_t ret;

	osThreadEnterCritical();
	{
		ret = p->priority;
	}
	osThreadExitCritical();

	return ret;
}

void
osThreadSetPriority( osHandle_t thread, osCounter_t priority )
{
	Thread_t* p = (Thread_t*) thread;
	PrioritizedList_t* list;

	OS_ASSERT(thread);

	if( thread == 0 )
		p = currentThread;

	osThreadEnterCritical();
	{
		if( priority != p->priority )
		{
			list = (PrioritizedList_t*) ( p->schedulerListItem.list );

			/* schedulerListItem might not be in any list, so it's necessary to check */
			if( list != NULL )
			{
				/* this will remove the list item from both prioritizedList and NotPrioritizedList,
				 * whichever the item was in. */
				notPrioritizedList_remove( &p->schedulerListItem );

				p->priority = priority;
				prioritizedList_insert( (PrioritizedListItem_t*) ( &p->schedulerListItem ), list );
			}
			else
				p->priority = priority;
		}
	}
	osThreadExitCritical();
}

osHandle_t
osThreadGetCurrentHandle( void )
{
	osHandle_t ret;
	osThreadEnterCritical();
	{
		ret = (osHandle_t) currentThread;
	}
	osThreadExitCritical();
	return ret;
}

void
osThreadDelay( osCounter_t timeout )
{
	/* the thread have to be in a ready state to block */
	OS_ASSERT( currentThread->state == OSTHREAD_READY );

	/* the thread have to be in the ready list */
	OS_ASSERT( currentThread->schedulerListItem.list == (void* )( &threads_ready ) );

	osThreadEnterCritical();
	{
		/* Preventing a 0 timeout from causing the thread to undesirably block permanently. */
		if( timeout != 0 )
		{
			thread_blockCurrent( NULL, timeout, NULL );
		}
	}
	osThreadExitCritical();
}

void
osThreadYield( void )
{
	osThreadEnterCritical();
	{
		nextThread = (Thread_t*) nextThread->schedulerListItem.next->container;
		thread_setNew();

		if( currentThread != nextThread )
			port_yield();
	}
	osThreadExitCritical();
}

