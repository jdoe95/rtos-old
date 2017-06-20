/*********************************************************************
 * INLINED LIST FUNCTIONS
 *
 * AUTHOR BUYI YU
 *  1917804074@qq.com
 *
 * (C) 2017
 *
 *  You should have received an open source user license.
 * 	ABOUT USAGE, MODIFICATION, COPYING, OR DISTRIBUTION, SEE LICENSE.
 *
 * 	List functions that would otherwise be more expensive to be defined as
 * 	not inlined.
 *********************************************************************/
#ifndef HFD62E024_D3DE_4A7E_A198_936CA6BEB679
#define HFD62E024_D3DE_4A7E_A198_936CA6BEB679

INLINE void
notPrioritizedList_init( NotPrioritizedList_t* list )
{
	list->first = NULL;
}

INLINE void
prioritizedList_init( PrioritizedList_t* list )
{
	list->first = NULL;
}

INLINE void
notPrioritizedList_insertBeforeByCookie( NotPrioritizedListItem_t* item, NotPrioritizedListItem_t* position )
{
	listItemCookie_insertBefore( (ListItemCookie_t*) item, (ListItemCookie_t*) position );
}

INLINE void
notPrioritizedList_insertAfterByCookie( NotPrioritizedListItem_t* item, NotPrioritizedListItem_t* position )
{
	listItemCookie_insertAfter( (ListItemCookie_t*) item, (ListItemCookie_t*) position );
}

INLINE void
notPrioritizedList_removeByCookie( NotPrioritizedListItem_t* item )
{
	listItemCookie_remove( (ListItemCookie_t*) item );
}

INLINE void
prioritizedList_insertBeforeByCookie( PrioritizedListItem_t* item, PrioritizedListItem_t* position )
{
	listItemCookie_insertBefore( (ListItemCookie_t*) item, (ListItemCookie_t*) position );
}

INLINE void
prioritizedList_insertAfterByCookie( PrioritizedListItem_t* item, PrioritizedListItem_t* position )
{
	listItemCookie_insertAfter( (ListItemCookie_t*) item, (ListItemCookie_t*) position );
}

INLINE void
prioritizedList_removeByCookie( PrioritizedListItem_t* item )
{
	listItemCookie_remove( (ListItemCookie_t*) item );
}

#endif /* HFD62E024_D3DE_4A7E_A198_936CA6BEB679 */
