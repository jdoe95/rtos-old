/** *********************************************************************
 * @file
 * @brief Thread related inline functions
 * @author John Doe (jdoe35087@gmail.com)
 * @details This file contains the implementation of thread related
 * inline functions.
 ***********************************************************************/
#ifndef HBB04B7C2_6C51_466E_A8C0_054EFD919F41
#define HBB04B7C2_6C51_466E_A8C0_054EFD919F41

/**
 * @brief Reschedules a thread
 * @details This function will check if the currentThread has the highest
 * priority in the ready list. If not, it will set nextThread to be the the
 * thread which has the highest priority in the ready list, so that it can
 * be swapped into the CPU by the context switcher by calling @ref port_yield.
 * @note this function must be used in a critical section.
 */
OS_INLINE void
thread_setNew( void )
{
	/* this function has to be called in a critical section because it accesses global resources. */
	OS_ASSERT( criticalNesting );

	/* the next thread pointer have to be in the ready list */
	OS_ASSERT( (void*)nextThread->schedulerListItem.list == &threads_ready );

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
