/** **********************************************************
 * @file
 * @brief Operating System Control and Status Functions
 * @author John Doe (jdoe35087@gmail.com)
 * @details This file contains the functions for the control and
 * status of the operating system. It also contains the system
 * tick handler.
 * ************************************************************/
#include "../includes/config.h"
#include "../includes/types.h"
#include "../includes/global.h"
#include "../includes/functions.h"

/**
 * @brief The idle thread stack
 * @details The idle thread stack is defined as static instead of
 * dynamically allocated on the heap is because the idle thread
 * never returns, its stack will never get freed. Why not define
 * it as static?
 */
static osByte_t idleThreadStack[OS_IDLE_THREAD_STACK_SIZE];

/**
 * @brief Initializes the operating system
 * @details This function initializes the operating system.
 * @note This function must be called before using any other operating
 * system API functions.
 */
void
osInit( void )
{
	/* initialize the global counters. Although this might already
	 * been done when the BSS section is cleared. */
	systemTime = 0;
	criticalNesting = 0;

	/* initialize the heap */
	memory_heapInit();

	/* Creates the initial heap memory block */
	MemoryBlock_t* block = (MemoryBlock_t*) OS_HEAP_START_ADDR;
	osCounter_t size = (osByte_t*) OS_HEAP_END_ADDR - (osByte_t*) OS_HEAP_START_ADDR;

	memory_blockCreate( block, size );
	memory_blockInsertToHeap( block );

	/* initialize the kernel memory list */
	memory_listInit( & kernelMemoryList );

	/* initialize the scheduling lists */
	prioritizedList_init( & threads_timed );
	prioritizedList_init( & threads_ready );

	/* initialize the timer priority list */
	notPrioritizedList_init( & timerPriorityList );

	/* create the idle thread */
	thread_init( &idleThread );
	idleThread.stackMemory = idleThreadStack;
	/* fill the stack with an initial fake thread context, which will be
	 * loaded into the CPU by the context switcher */
	idleThread.PSP = port_makeFakeContext( idleThreadStack, OS_IDLE_THREAD_STACK_SIZE, port_idle, 0 );
	idleThread.priority = OS_PRIO_LOWEST;
	idleThread.state = OSTHREAD_READY;

	/* add the idle thread to the ready list */
	prioritizedList_insert( (PrioritizedListItem_t*) ( &idleThread.schedulerListItem ), &threads_ready );

	/* initialize the variables used for scheduling */
	currentThread = &idleThread;
	nextThread = &idleThread;
}

/**
 * @brief Starts the operating system
 * @details This function starts the operating system and jumps to
 * the context of the first thread. It should never return.
 */
void
osStart( void )
{
	/* load current thread pointer from the highest priority thread in the ready list
	 * currentThread will be loaded into the CPU by port_startKernel
	 */
	currentThread = (Thread_t*) ( threads_ready.first->container );
	port_startKernel();
}

/**
 * @brief The operating system heart beat interrupt handler
 * @details This function will be called periodically to switch between
 * the threads with the highest priority in the ready list.
 */
OS_INTERRUPT void
OS_TICK_HANDLER_NAME( void )
{
	Thread_t* thread;
	PrioritizedListItem_t* listItem;

	osThreadEnterCritical();
	{
		/* increment system time */
		systemTime++;

		/* ready the timed-out threads in the timing list */
		while( threads_timed.first != NULL )
		{
			/* get the first thread */
			listItem = threads_timed.first;
			thread = (Thread_t*) listItem->container;

			if( systemTime >= listItem->value )
				thread_makeReady( thread );

			else
				/* since the items in the threads_timed list are
				 * organized in incremental order by their timeout value,
				 * it is not necessary to continue */
				break;
		}

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
		/* there's at least one thread (the idle thread) in the ready list */
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

/**
 * @brief Returns the current operating system time
 * @return The current global operating system time in ticks elapsed since the
 * start of the operating system.
 * @details This function returns the global operating system time, expressed in
 * ticks elapsed since the start of the operating system. With a 32 bit unsigned
 * integer counter and an 100 Hz tick frequency, it will overflow in about 1.36 years,
 * or 1 year and 4 months.
 */
osCounter_t
osGetTime( void )
{
	osCounter_t ret;

	/* a critical section is necessary since the read on osCounter might
	 * take multiple instructions to complete */
	osThreadEnterCritical();
	{
		ret = systemTime;
	}
	osThreadExitCritical();

	return ret;
}
