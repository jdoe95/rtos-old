/*********************************************************************
 * INLINED THREAD FUNCTIONS
 *
 * AUTHOR BUYI YU
 *  1917804074@qq.com
 *
 * (C) 2017
 *
 *  You should have received an open source user license.
 * 	ABOUT USAGE, MODIFICATION, COPYING, OR DISTRIBUTION, SEE LICENSE.
 *
 * 	Thread functions that would otherwise be more expensive to be defined as
 * 	not inlined.
 *********************************************************************/
#ifndef HBB04B7C2_6C51_466E_A8C0_054EFD919F41
#define HBB04B7C2_6C51_466E_A8C0_054EFD919F41

/* reschedule a new thread for context switching. This is the core function of the scheduler
 * Global scheduler variables:
 * currentThread 		-- the thread that current context information should be saved to
 * nextThread			-- the thread that the context information should be loaded from, if context switching is requested
 * */
INLINE void
thread_setNew( void )
{
	/* this function has to be called in a critical section because it accesses global resources. */
	OS_ASSERT( criticalNesting );

	/* the next thread pointer have to be in the ready list */
	OS_ASSERT( nextThread->schedulerListItem.list == (void*)( & threads_ready ) );

	/* check if next thread's priority is the highest */
	if( nextThread->priority == threads_ready.first->value )
	{
		/* do nothing, nextThread will be loaded */
	}
	else /* next thread's priority is not the highest */
	{
		/* load the first highest priority thread */
		nextThread = (Thread_t*)( threads_ready.first->container );
	}
}


#endif /* HBB04B7C2_6C51_466E_A8C0_054EFD919F41 */
