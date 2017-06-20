/*******************************************************************
 * LIST FUNCTIONS
 *
 * AUTHOR BUYI YU
 *  1917804074@qq.com
 *
 * (C) 2017
 *
 * 	You should have received an open source user license.
 * 	ABOUT USAGE, MODIFICATION, COPYING, OR DISTRIBUTION, SEE LICENSE.
 *******************************************************************/
#include "../includes/config.h"
#include "../includes/functions.h"

/* list item cookie *************************************************************/
void
listItemCookie_insertBefore( ListItemCookie_t* cookie, ListItemCookie_t* position )
{
	cookie->prev = position->prev;
	cookie->next = position;
	position->prev->next = cookie;
	position->prev = cookie;
}

void
listItemCookie_insertAfter( ListItemCookie_t* cookie, ListItemCookie_t* position )
{
	cookie->next = position->next;
	cookie->prev = position;
	position->next->prev = cookie;
	position->next = cookie;
}

void
listItemCookie_remove( ListItemCookie_t* cookie )
{
	cookie->prev->next = cookie->next;
	cookie->next->prev = cookie->prev;
	cookie->next = cookie;
	cookie->prev = cookie;
}

/* not prioritized list ***********************************************************/
void
notPrioritizedList_itemInit( NotPrioritizedListItem_t* item, void *container )
{
	item->prev = item;
	item->next = item;
	item->container = container;
	item->list = NULL;
}

void
notPrioritizedList_insert( NotPrioritizedListItem_t* item, NotPrioritizedList_t* list )
{
	OS_ASSERT( item->list == NULL );

	if( list->first == NULL )
	{
		list->first = item;
		item->prev = item;
		item->next = item;
	}
	else
		/* insert as last item */
		notPrioritizedList_insertBeforeByCookie( item, list->first );

	item->list = list;
}

void
notPrioritizedList_remove( NotPrioritizedListItem_t* item )
{
	OS_ASSERT( item->list != NULL );

	if( item == item->list->first )
	{
		/* 'first' should always point into the list */
		item->list->first = item->list->first->next;

		if( item == item->list->first )
			/* this means that this item is the only item in the list */
			item->list->first = NULL;
	}

	notPrioritizedList_removeByCookie( item );
	item->list = NULL;
}

NotPrioritizedListItem_t*
notPrioritizedList_findContainer( const void* container, NotPrioritizedList_t* list )
{
	NotPrioritizedListItem_t* i;

	if( list->first != NULL )
	{
		/* the list is not empty */

		i = list->first;

		do
		{
			if( i->container == container )
				return i;

			i = i->next;
		} while( i != list->first );
	}

	return NULL;
}

/* prioritized list **********************************************************************/
void
prioritizedList_itemInit( PrioritizedListItem_t* item, void *container, osCounter_t value )
{
	item->prev = item;
	item->next = item;
	item->container = container;
	item->list = NULL;
	item->value = value;
}

void
prioritizedList_insert( PrioritizedListItem_t* item, PrioritizedList_t* list )
{
	PrioritizedListItem_t* i;

	OS_ASSERT( item->list == NULL );

	if( list->first == NULL )
	{
		list->first = item;
		item->prev = item;
		item->next = item;
	}
	else if( item->value >= list->first->prev->value )
	{
		/* insert as last item */
		prioritizedList_insertBeforeByCookie( item, list->first );
	}
	else if( item->value < list->first->value )
	{
		/* insert as first item */
		prioritizedList_insertBeforeByCookie( item, list->first );
		list->first = item;
	}
	else
	{
		/* search starts from second item */
		i = list->first->next;
		do
		{
			/* insert just before the item that has a greater value */
			if( item->value < i->value )
				break;

			i = i->next;

		} while(true); /* equivalent to while(i != list->first) */

		prioritizedList_insertBeforeByCookie( item, i );
	}

	item->list = list;
}

void
prioritizedList_remove( PrioritizedListItem_t* item )
{
	OS_ASSERT( item->list != NULL );

	if( item == item->list->first )
	{
		/* 'first' should always point into the list */
		item->list->first = item->list->first->next;

		/* this means that this item is the only item in the list */
		if( item == item->list->first )
			item->list->first = NULL;
	}

	prioritizedList_removeByCookie( item );
	item->list = NULL;
}

PrioritizedListItem_t*
prioritizedList_findContainer( const void* container, PrioritizedList_t* list )
{
	PrioritizedListItem_t* i;

	if( list->first != NULL )
	{
		/* the list is not empty */

		i = list->first;
		do
		{
			if( i->container == container )
				return i;

			i = i->next;
		} while( i != list->first );
	}

	return NULL;
}
