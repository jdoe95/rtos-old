/** *********************************************************************
 * @file
 * @brief Thread related functions
 * @author John Doe (jdoe35087@gmail.com)
 * @details This file contains the implementation of thread related functions.
 ***********************************************************************/
#include "../includes/config.h"
#include "../includes/types.h"
#include "../includes/global.h"
#include "../includes/functions.h"

/**
 * @brief Called when a thread used a return statement
 * @details This function will be called when the thread used a
 * return statement to terminate itself. This function simply calls
 * @ref osThreadDelete to destroy the thread itself and release the
 * resources. In order for this function to work, this function
 * should be stored in the initial fake stack frame created during
 * the creation of the thread, so that when a return statement is
 * called, the address of this function will be popped into the
 * CPU's program counter.
 */
void
threadReturnHook( void )
{
	/* when this function is executing, current thread should be in
	 * the ready state, because it needs to delete itself. */
	OS_ASSERT( currentThread->state == OSTHREAD_READY );

	/* Thread exited. Pass 0 to destroy current thread */
	osThreadDelete(0);
}

/**
 * @brief Initializes the thread control block.
 * @param thread pointer to the thread control block to be initialized
 * @details This function initializes the structures in the thread control
 * block to proper values. It does not fill all the fields because
 * some fields will be filled by @ref osThreadCreate
 */
void
thread_init( Thread_t* thread )
{
	notPrioritizedList_itemInit( &thread->schedulerListItem, thread );
	prioritizedList_itemInit( &thread->timerListItem, thread, 0 );
	memory_listInit( &thread->localMemory );
	thread->wait = NULL;
}

/**
 * @brief Readies a thread
 * @param thread pointer to the thread control block of the thread that is
 * to be readied
 * @details this function will remove the thread from some resource's waiting
 * list and restore the thread to a ready state.
 * @note this function must be used inside a critical section
 */
void
thread_makeReady( Thread_t* thread )
{
	/* this function has to be called in a critical section because it accesses global resources */
	OS_ASSERT( criticalNesting );

	/* the thread to be readied must not be in the ready list */
	OS_ASSERT( currentThread->state != OSTHREAD_READY );
	OS_ASSERT( thread->schedulerListItem.list != (void* )&threads_ready );

	/* remove the schedulerListItem from some resource's waiting list, if any */
	if( thread->schedulerListItem.list != NULL )
	{
		/* this will remove the list item from both prioritizedList and NotPrioritizedList,
		 * whichever the item was in. */
		list_remove( &thread->schedulerListItem );
	}

	/* If the thread was in a timed blocking, we also need to remove it from the system
	 * timeout list. */
	if( thread->timerListItem.list != NULL )
		list_remove( &thread->timerListItem );

	/* remove the docked wait struct. */
	thread->wait = NULL;

	/* insert the thread into the ready list, and set its state to ready */
	prioritizedList_insert( (PrioritizedListItem_t*) ( &thread->schedulerListItem ), &threads_ready );
	thread->state = OSTHREAD_READY;
}


/**
 * @brief Readies all threads in a list
 * @param list the list in which all threads need to be readied, this cannot be the
 * ready list.
 * @details This function calls @ref thread_makeReady on every item in the specified list,
 * until all the threads are moved to the ready list.
 * @note this function must be used in a critical section
 */
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

/**
 * @brief Removes @ref currentThread from the ready list and put it into an optional
 * resource waiting list
 * @param list the waiting list the thread to be put into. This cannot be the ready
 * list. NULL can be passed if the thread is not to be put into any waiting list.
 * @param timeout maximum amount of time to block for a resource. Pass 0 for indefinite
 * @param wait optional wait structure to be docked onto the thread's control block
 * @details This function removes current thread from the ready list and put it
 * into an optional waiting list. The time out for the thread to be in that list
 * can be optionally specified as timeout. After the timeout, the thread will
 * be automatically readied. An optional wait structure can be specified to dock
 * onto the thread's control block in order to receive information regarding the
 * details of the resource the thread is about to be waiting for.
 * @note this function must be used in a critical section
 */
void
thread_blockCurrent( PrioritizedList_t* list, osCounter_t timeout, void* wait )
{
	/* the critical nesting counter is per-thread. define the critical nesting counter
	 * on the thread's stack */
	osCounter_t criticalNestingSave;

	/* this function has to be called in a critical section because it accesses
	 * global resources. */
	OS_ASSERT( criticalNesting );

	/* current thread is expected to be ready  */
	OS_ASSERT( currentThread->state == OSTHREAD_READY );
	OS_ASSERT( currentThread->schedulerListItem.list == (void* )( &threads_ready ) );

	/* since currentThread is about to be removed from the ready list,
	 * if nextThread is the same as currentThread (happens typically
	 * after a context switch but before the next re-schedule), make sure
	 * that nextThread stays in the ready list after removing currentThread */
	if( currentThread == nextThread )
	{
		/* there's at least one thread (the idle thread) in the ready list */
		nextThread = (Thread_t*)( nextThread->schedulerListItem.next->container );
	}

	/* this will remove the list item from both prioritizedList and NotPrioritizedList,
	 * whichever the item was in. */
	list_remove( &currentThread->schedulerListItem );

	/* change the state of the thread to blocked */
	currentThread->state = OSTHREAD_BLOCKED;

	/* if any, insert currentThread into a blocking list */
	if( list != NULL )
		prioritizedList_insert( (PrioritizedListItem_t*) &currentThread->schedulerListItem, list );

	/* a finite timeout value, add to system timing list */
	if( timeout != 0 )
	{
		/* calculate next time for next wake up. this even works if systemTime is about to overflow,
		 * because the calculated time for next wake up overflows too */
		currentThread->timerListItem.value = timeout + systemTime;

		/* upon wake up, the timerListItem will be automatically removed from the list */
		prioritizedList_insert( &currentThread->timerListItem, &threads_timed );
	}

	/* dock the wait struct onto the thread control block */
	currentThread->wait = wait;

	/* schedule a new thread */
	thread_setNew();
	OS_ASSERT( currentThread != nextThread );

	/* the critical nesting counter is stored per-thread, save criticalNesting to stack,
	 * set critical nesting counter to zero because the next thread to be scheduled
	 * might be resuming from a running state, in which the critical nesting counter is
	 * assumed to be zero. */
	criticalNestingSave = criticalNesting;
	criticalNesting = 0;

	/* open a window for the context switcher */
	port_enableInterrupts();
	{
		port_yield();

		/* the thread resumes here */
	}
	port_disableInterrupts();
	
	/* restore the critical nesting from the thread's stack */
	OS_ASSERT( criticalNestingSave );
	criticalNesting = criticalNestingSave;

	/* the thread should be in the ready list when resumed from port_yield() */
	OS_ASSERT( currentThread->schedulerListItem.list == (void* )( &threads_ready ) );

	/* the thread should be in the ready state when resumed from port_yield */
	OS_ASSERT( currentThread->state == OSTHREAD_READY );

	/* the timer list item should be removed */
	OS_ASSERT( currentThread->timerListItem.list == NULL );
}

/**
 * @brief Enters a critical section
 * @details This function is used to enter a critical section where interrupts
 * (and scheduling) are disabled in order to perform a few short operations
 * on global data structures. It supports nesting, which means
 * @ref osThreadExitCritical must be called for equal amount of times as
 * this function.
 *
 * @see osThreadExitCritical
 *
 * @note contexts in which this function can be used
 * 	- Yes: an interrupt context
 * 	- Yes: main stack context before the kernel started
 * 	- Yes: thread contexts
 */
void
osThreadEnterCritical( void )
{
	/* enter critical section before accessing global resource 'criticalNesting' */
	port_disableInterrupts();
	criticalNesting++;
}

/**
 * @brief Exits a critical section
 * @details This function is used to exit a critical section where interrupts
 * (and scheduling) are disabled in order to perform a few short operations
 * on global data structures. It supports nesting, which means
 * @ref osThreadEnterCritical must be called for equal amount of times as
 * this function.
 *
 * @see osThreadEnterCritical
 *
 * @note contexts in which this function can be used
 * 	- Yes: an interrupt context
 * 	- Yes: main stack context before the kernel started
 * 	- Yes: thread contexts
 */
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

/**
 * @brief Externally changes the critical section nesting counter
 * @param counter the new critical section nesting counter
 * @details This function is used to externally change the critical nesting
 * counter. This function is barely used. Improper use of this function
 * will cause chaos in the system. It is only invented to integrate
 * critical section functions in outside libraries.
 *
 * @note contexts in which this function can be used
 * 	- Yes: an interrupt context
 * 	- Yes: main stack context before the kernel started
 * 	- Yes: thread contexts
 */
void
osThreadSetCriticalNesting( osCounter_t counter )
{
	/* enter critical section before modifying critical nesting counter */
	port_disableInterrupts();
	criticalNesting = counter;

	if( counter == 0 )
		port_enableInterrupts();
}

/**
 * @brief Returns the critical section nesting counter
 * @return the critical section nesting counter
 * @details This function is used to obtain the value of the critical nesting
 * counter. This function is barely used. It is only invented to integrate
 * critical section functions in outside libraries.
 *
 * @note contexts in which this function can be used
 * 	- Yes: an interrupt context
 * 	- Yes: main stack context before the kernel started
 * 	- Yes: thread contexts
 */
osCounter_t
osThreadGetCriticalNesting( void )
{
	osCounter_t ret;

	/* enter critical section before reading critical nesting counter */
	port_disableInterrupts();
	ret = criticalNesting;

	if( ret == 0 )
		port_disableInterrupts();

	return ret;
}

/**
 * @brief Creates a thread
 * @param priority the priority of the thread to be created
 * @param code the code (a function pointer casted to osCode_t) of the thread
 * @param stackSize the stack size of the thread
 * @param argument the argument to pass to the thread
 * @return handle to the created thread, if the thread was created successfully;
 * 0, if the thread was not created.
 *
 * @details
 * The stack will be allocated from the heap and put to kernel memory list when
 * calling this function. It is mandatory for the user to ensure that the thread
 * does not exceed its stack usage watermark, since key data might be corrupted
 * by a stack overflow. If errors were encountered in creating the thread, usually
 * due to lack of resources, the function will fail and return 0.
 *
 * This function can be called before calling @ref osStart. If this is the
 * case, the created thread will start running as soon as calling @ref osStart.
 * Otherwise, the newly created thread will not start running immediately. It
 * will be scheduled to run on the next scheduling event. The scheduling event
 * is usually generated by the heart beat timer interrupt service routine, or
 * some status change in system resources, like those in semaphores and queues,
 * etc. If immediate execution is required, call @ref osThreadYield after creating
 * the thread. Note that the mentioned function cannot be called before calling
 * @ref osStart.
 *
 * @note contexts in which this function can be used
 * 	- Yes: an interrupt context
 * 	- Yes: main stack context before the kernel started
 * 	- Yes: thread contexts
 *
 * 	@see osThreadDelete
 */
osHandle_t
osThreadCreate( osCounter_t priority, osCode_t code, osCounter_t stackSize, const void* argument )
{
	Thread_t* thread;
	osByte_t* stackMemory;

	/* allocate a thread control block, put it into kernel memory list */
	/* the function is thread safe */
	osThreadEnterCritical();
	thread = (Thread_t*) memory_allocateFromHeap( sizeof(Thread_t), &kernelMemoryList );
	osThreadExitCritical();

	/* check the allocation */
	if( thread == NULL )
	{
		/* allocation failed, halt here if in debug mode, return 0 if in release */
		OS_ASSERT(0);
		return 0;
	}

	/* allocate stack memory for the thread, put it into kernel memory list */
	/* the function is thread safe */
	osThreadEnterCritical();
	stackMemory = (osByte_t*) memory_allocateFromHeap( stackSize, &kernelMemoryList );
	osThreadExitCritical();

	/* check the allocation */
	if( stackMemory == NULL )
	{
		/* allocation failed, release the previously allocated resources, halt here if
		 * in debug mode, return 0 if in release.
		 */
		osThreadEnterCritical();
		memory_returnToHeap( thread, & kernelMemoryList );
		osThreadExitCritical();

		OS_ASSERT(0);
		return 0;
	}

	/* initialize the thread control block */
	thread_init( thread );
	/* fill the stack with an initial fake thread context, which will be loaded
	 * into the CPU by the context switcher */
	thread->PSP = port_makeFakeContext( stackMemory, stackSize, code, argument );
	thread->priority = priority;
	thread->stackMemory = stackMemory;

	/* a critical section is necessary since the function modifies global structures */
	osThreadEnterCritical();
	thread_makeReady( thread );
	osThreadExitCritical();
	return (osHandle_t) thread;
}
/**
 * @brief Returns the current state of a thread
 * @param h the handle to the thread whose state is to be returned, if the
 * handle is 0, the function will always return OSTHREAD_READY.
 * @return the state of the thread
 * @note contexts in which this function can be used
 * 	- Yes: an interrupt context
 * 	- No: main stack context before the kernel started
 * 	- Yes: thread contexts
 */
osThreadState_t
osThreadGetState( osHandle_t h )
{
	Thread_t* p = (Thread_t*) h;

	if( h == 0 )
		return OSTHREAD_READY;

	osThreadState_t ret;
	osThreadEnterCritical();
	{
		ret = p->state;
	}
	osThreadExitCritical();

	return ret;
}

/**
 * @brief Deletes a thread
 * @param h handle to the thread to be deleted. 0 can be used if deleting
 * current thread.
 * @details Threads in all states can be deleted by calling this function.
 * The unfreed memory allocated my calling @ref osMemoryAllocate
 * during the life time of the thread will automatically be released. After
 * calling this function, the handle will be invalid and should not be used
 * again.
 *
 * @note contexts in which this function can be used
 * 	- Yes: an interrupt context
 * 	- Yes: main stack context before the kernel started
 * 	- Yes: thread contexts
 *
 * @see osThreadCreate
 */
void
osThreadDelete( osHandle_t h )
{
	Thread_t* p = (Thread_t*) h;
	MemoryBlock_t* block;

	if( h == 0 )
		p = currentThread;

	osThreadEnterCritical();
	{
		/* remove the scheduler list item from its list, if any */
		if( p->schedulerListItem.list != NULL )
		{
			/* if nextThread is the same as currentThread (happens typically
			 * after a context switch but before the next re-schedule), make sure
			 * that nextThread stays in the ready list after removing currentThread */
			if( p == nextThread )
				/* there's at least one thread (the idle thread) in the ready list */
				nextThread = (Thread_t*)( nextThread->schedulerListItem.next->container );

			/* this will remove the list item from both prioritizedList and NotPrioritizedList,
			 * whichever the schedulerListItem is in. */
			list_remove( &p->schedulerListItem );
		}

		/* remove the scheduler from the timing list, if it is in the list  */
		if( p->timerListItem.list != NULL )
			list_remove( &p->timerListItem );


		/* Free all unfreed memory blocks allocated when osMemoryAllocate was called */
		while( p->localMemory.first != NULL )
		{
			/* point to a memory block */
			block = p->localMemory.first;

			memory_blockRemoveFromMemoryList( block, & p->localMemory );
			memory_returnBlockToHeap( block );
		}

		/* after this, if deleting current thread, the context switcher will still try
		 * to a stack frame data onto the thread's stack which is located in
		 * kernelMemoryList at the moment. If a stack overflow occurs at that stage,
		 * the kernelMemoryList can still be corrupted */
		memory_returnToHeap( p->stackMemory, & kernelMemoryList );
		memory_returnToHeap( p, & kernelMemoryList );

		/* load another thread if deleting current thread */
		if( p == currentThread )
		{
			/* current thread is no longer in the ready list, thus out of consideration
			 * by the scheduling function. */
			thread_setNew();
			port_yield();
		}
	}
	osThreadExitCritical();
}

/**
 * @brief Suspends a thread
 * @param h handle to the thread to be suspended, 0 can be passed
 * if suspending current thread.
 * @details The function can be called on an already suspended
 * thread, in which case the function will have no effect.
 * If the thread blocked on a resource before this function
 * is called, it will be removed from the waiting list of the
 * resource and from the system timing list. When the thread gets
 * resumed, the block will fail and return false.
 *
 * @note contexts in which this function can be used
 * 	- Yes: an interrupt context
 * 	- Yes: main stack context before the kernel started
 * 	- Yes: thread contexts
 *
 * @see osThreadResume
 */
void
osThreadSuspend( osHandle_t h )
{
	Thread_t* p = (Thread_t*) h;

	/* the critical nesting counter is per-thread, so store
	 * it on the stack */
	osCounter_t criticalNestingSave;

	if( h == 0 )
		p = currentThread;

	osThreadEnterCritical();
	{
		/* support for suspending multiple times */
		if( p->state != OSTHREAD_SUSPENDED )
		{
			/* remove the scheduler list item from its list, if any */
			if( p->schedulerListItem.list != NULL )
			{
				/* if nextThread is the same as currentThread (happens typically
				 * after a context switch but before the next re-schedule), make sure
				 * that nextThread stays in the ready list after removing currentThread */
				if( p == nextThread )
					/* there's at least one thread (the idle thread) in the ready list */
					nextThread = (Thread_t*)( nextThread->schedulerListItem.next->container );

				/* this will remove the list item from both prioritizedList and NotPrioritizedList,
				 * whichever the schedulerListItem was in. */
				list_remove( &p->schedulerListItem );
			}

			/* change the state of the thread */
			p->state = OSTHREAD_SUSPENDED;

			/* remove the scheduler from the timing list, if it is in the list  */
			if( p->timerListItem.list != NULL )
				list_remove( &p->timerListItem );

			/* remove docked wait struct */
			p->wait = NULL;

			/* only needs to yield when suspending current thread */
			if( p == currentThread )
			{
				thread_setNew();
				OS_ASSERT( currentThread != nextThread );

				/* the critical nesting counter is stored per-thread,
				 * save criticalNesting (to the stack of current thread). Next thread's
				 * critical nesting counter will be restored to the system global space
				 * when next thread resumes */
				criticalNestingSave = criticalNesting;

				/* set critical nesting counter to zero because the next thread to be scheduled
				 * might be resuming from a running state, in which the critical nesting counter is
				 * assumed to be zero. */
				criticalNesting = 0;

				/* open an interrupt-enabled window for the context switcher */
				port_enableInterrupts();
				{
					port_yield();

					/* the thread resumes here if osThreadResume is called */
				}
				port_disableInterrupts();

				/* restore the critical nesting counter to system global space */
				criticalNesting = criticalNestingSave;
			}
		} /* if not suspended */
	}
	osThreadExitCritical();
}

/**
 * @brief Resumes a suspended thread
 * @param h handle to the thread to be resumed. Since resuming current thread
 * makes no sense, this parameter has to be non zero.
 * @details This function is used to resume a suspended thread. It can also
 * be called on a thread that is not in a suspended state, in which case the
 * function will have no effect.
 *
 * @note contexts in which this function can be used
 * 	- Yes: an interrupt context
 * 	- No: main stack context before the kernel started
 * 	- Yes: thread contexts
 */
void
osThreadResume( osHandle_t h )
{
	Thread_t* p = (Thread_t*) h;

	OS_ASSERT(h);

	osThreadEnterCritical();
	{
		/* adds support for resuming multiple times */
		if( p->state == OSTHREAD_SUSPENDED )
		{
			/* ready this thread */
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

/**
 * @brief Returns the priority of a thread
 * @param h handle to the thread whose priority is to be returned, 0 can be
 * passed to check the priority of current thread.
 * @return the priority of the thread
 *
 * @note contexts in which this function can be used
 * 	- Yes: an interrupt context
 * 	- Yes: main stack context before the kernel started
 * 	- Yes: thread contexts
 */
osCounter_t
osThreadGetPriority( osHandle_t h )
{
	Thread_t* p = (Thread_t*) h;

	if( h == 0 )
		p = currentThread;

	osCounter_t ret;

	/* a critical section is needed in case read operations on osCounter_t
	 * takes multiple instructions to complete */
	osThreadEnterCritical();
	{
		ret = p->priority;
	}
	osThreadExitCritical();

	return ret;
}

/**
 * @brief Changes the priority of a thread
 * @param h the handle to the thread whose priority is to be changed, 0 can
 * be passed if changing the priority of current thread
 * @param priority the new priority of the thread
 * @details A reschedule will happen immediately after the priority modification
 * if the new priority is the highest priority in the system.
 *
 * @note contexts in which this function can be used
 * 	- Yes: an interrupt context
 * 	- No: main stack context before the kernel started
 * 	- Yes: thread contexts */
void
osThreadSetPriority( osHandle_t h, osCounter_t priority )
{
	Thread_t* p = (Thread_t*) h;
	PrioritizedList_t* list;

	if( h == 0 )
		p = currentThread;

	osThreadEnterCritical();
	{
		/* since changing priority takes a long time, but checking the priority does not,
		 * check if the priority value is really different before performing the actions. */
		if( priority != p->priority )
		{
			list = (PrioritizedList_t*) ( p->schedulerListItem.list );

			/* remove the schedulerListItem from its list, if any */
			if( list != NULL )
			{
				/* this will remove the list item from both prioritizedList and NotPrioritizedList,
				 * whichever the item was in. */
				list_remove( &p->schedulerListItem );

				/* change the priority */
				p->priority = priority;

				/* reinsert the schedulerListItem back to its list */
				prioritizedList_insert( (PrioritizedListItem_t*) ( &p->schedulerListItem ), list );
			}
			else
				/* if the schedulerListItem is not in a list, change the value directly */
				p->priority = priority;

			/* re-schedule immediately if higher priority is set */
			if( priority < currentThread->priority )
			{
				thread_setNew();
				port_yield();
			}
		}
	}
	osThreadExitCritical();
}

/**
 * @brief Returns the handle of current thread
 * @return the handle of current thread
 *
 * @note contexts in which this function can be used
 * 	- Yes: an interrupt context
 * 	- No: main stack context before the kernel started
 * 	- Yes: thread contexts
 */
osHandle_t
osThreadGetCurrentHandle( void )
{
	osHandle_t ret;

	/* a critical section is needed in case the read operation on osHandle_t
	 * takes multiple instructions to complete */
	osThreadEnterCritical();
	{
		ret = (osHandle_t) currentThread;
	}
	osThreadExitCritical();
	return ret;
}

/**
 * @brief Yields the CPU for an amount of time
 * @param timeout the non-zero timeout after which current thread will be readied,
 * in ticks
 * @details This function temporarily yields the CPU usage from current thread to
 * other threads for an amount of time. This is useful in implementing "blinky".
 * Usually blinky applications will use busy wait functions, in which loop structures
 * are usually used to execute NOP instructions to waste CPU cycles to achieve delay.
 * But in this case, when calling this function, the CPU context will be given to
 * other threads temporarily, which allows very efficient CPU utilization.
 *
 * Note that the delay might not be accurate since two conditions have to be met
 * before the thread regains rights to the CPU.
 * - the specified timeout must have elapsed
 * - there must be no other threads in the ready list that has a higher priority than
 * that of this thread.
 *
 * @note contexts in which this function can be used
 * 	- No: an interrupt context
 * 	- No: main stack context before the kernel started
 * 	- Yes: thread contexts
 */
void
osThreadDelay( osCounter_t timeout )
{
	/* the thread have to be in a ready state to block */
	OS_ASSERT( currentThread->state == OSTHREAD_READY );
	OS_ASSERT( currentThread->schedulerListItem.list == (void* )( &threads_ready ) );

	osThreadEnterCritical();
	{
		/* Preventing a 0 timeout from causing the thread to undesirably block permanently,
		 * since thread_blockCurrent treats this as blocking indefinitely */
		if( timeout != 0 )
		{
			thread_blockCurrent( NULL, timeout, NULL );
		}
	}
	osThreadExitCritical();
}

/**
 * @brief Actively yields the CPU for other threads
 * @details This function is usually used when the priority of current thread
 * is the highest and the current thread wish to yield the CPU to other ready threads with
 * the same priority in the system. It can be used to implement round-robin scheduling
 * in a tickless environment.
 *
 * Note that if current thread is the only ready thread with the highest priority, this
 * function will have no effect. The execution context will remain on current thread in
 * this case.
 *
 * @note contexts in which this function can be used
 * 	- No: an interrupt context
 * 	- No: main stack context before the kernel started
 * 	- Yes: thread contexts
 */
void
osThreadYield( void )
{
	osThreadEnterCritical();
	{
		/* Perform round-robin scheduling
		 *
		 * The kernel allows multiple threads with same priority. All ready threads
		 * with the highest priority are organized at the front of threads_ready, and CPU]
		 * time will be shared among them.
		 *
		 * set the nextThread pointer to the next thread after current thread,
		 * and call thread_setNew to re-schedule. Since thread_setNew will automatically
		 * check the priority of nextThread, nextThread will be set back to the
		 * beginning of the list if its priority is no longer the highest.
		 */

		/* next thread have to be in the ready list */
		OS_ASSERT( nextThread->schedulerListItem.list == (void*)(&threads_ready) );

		/* move nextThread to the next item, and call the scheduler function to check if
		 * it should be run next. */
		nextThread = (Thread_t*)( nextThread->schedulerListItem.next->container );

		/* next thread have to be in the ready list */
		OS_ASSERT( nextThread->schedulerListItem.list == (void*)(&threads_ready) );

		thread_setNew();

		/* send yield request if scheduling gives a different thread */
		if( currentThread != nextThread )
			port_yield();
	}
	osThreadExitCritical();
}

