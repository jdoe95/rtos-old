/** *********************************************************************
 * @file
 * @brief List related inline functions
 * @author John Doe (jdoe35087@gmail.com)
 * @details This file contains the implementation of list related
 * inline functions.
 ***********************************************************************/
#ifndef HFD62E024_D3DE_4A7E_A198_936CA6BEB679
#define HFD62E024_D3DE_4A7E_A198_936CA6BEB679

/**
 * @brief Initializes a not prioritized list header
 * @param list pointer to the not prioritized list to be initialized
 */
OS_INLINE void
notPrioritizedList_init( NotPrioritizedList_t* list )
{
	list->first = NULL;
}

/**
 * @brief Initializes a prioritized list header
 * @param list pointer to the prioritized list to be initialized
 */
OS_INLINE void
prioritizedList_init( PrioritizedList_t* list )
{
	list->first = NULL;
}

#endif /* HFD62E024_D3DE_4A7E_A198_936CA6BEB679 */
