/** *********************************************************************
 * @file
 * @brief List related functions
 * @author John Doe (jdoe35087@gmail.com)
 * @details This file contains the implementation of list related functions.
 ***********************************************************************/
#include "../includes/config.h"
#include "../includes/functions.h"

/**
 * @brief Inserts an list item before the specified position
 * @param cookie pointer to the item to be inserted
 * @param position pointer to an item in the list that specifies the position
 * @details This function is called by higher level list functions. It only
 * updates the @ref listItemCookie.prev and @ref listItemCookie.next
 * fields in the list item, and should not be called directly.
 */
void
listItemCookie_insertBefore( void* cookie, void* position )
{
	ListItemCookie_t *pCookie = (ListItemCookie_t*)cookie,
			*pPosition = (ListItemCookie_t*)position;

	pCookie->prev = pPosition->prev;
	pCookie->next = pPosition;
	pPosition->prev->next = pCookie;
	pPosition->prev = pCookie;
}

/**
 * @brief Inserts an list item after the specified position
 * @param cookie pointer to the item to be inserted
 * @param position pointer to an item in the list that specifies the position
 * @details This function is called by higher level list functions. It only
 * updates the @ref listItemCookie.prev and @ref listItemCookie.next
 * fields in the list item, and should not be called directly.
 */
void
listItemCookie_insertAfter( void* cookie, void* position )
{
	ListItemCookie_t *pCookie = (ListItemCookie_t*)cookie,
			*pPosition = (ListItemCookie_t*)position;

	pCookie->next = pPosition->next;
	pCookie->prev = pPosition;
	pPosition->next->prev = pCookie;
	pPosition->next = pCookie;
}

/**
 * @brief Removes an list item from the list
 * @param cookie pointer to the item to be removed
 * @details This function is called by higher level list functions. It only
 * updates the @ref listItemCookie.prev and @ref listItemCookie.next
 * fields in the list item, and should not be called directly.
 */
void
listItemCookie_remove( void* cookie )
{
	ListItemCookie_t *pCookie = (ListItemCookie_t*)cookie;

	pCookie->prev->next = pCookie->next;
	pCookie->next->prev = pCookie->prev;
	pCookie->next = cookie;
	pCookie->prev = cookie;
}

/**
 * @brief Initializes a not prioritized list item
 * @param item pointer to the item to be initialized
 * @param container pointer to the structure that holds the list item
 * @details This function initializes the fields in @ref notPrioritizedListItem.
 */
void
notPrioritizedList_itemInit( NotPrioritizedListItem_t* item, void *container )
{
	item->prev = item;
	item->next = item;
	item->container = container;
	item->list = NULL;
}

/**
 * @brief Inserts a not prioritized list item to a not prioritized list
 * @param item pointer to the item to be inserted to the list
 * @param list pointer to the list
 * @note This function will always append the item to the end of the list.
 */
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
		listItemCookie_insertBefore( item, list->first );

	item->list = list;
}

/**
 * @brief Initializes a prioritized list item
 * @param item pointer to the prioritized list item to be initialized
 * @param container pointer to the structure that holds the list item
 * @param value the value of the list item
 * @details This function initializes the fields in @ref prioritizedListItem.
 */
void
prioritizedList_itemInit( PrioritizedListItem_t* item, void *container, osCounter_t value )
{
	item->prev = item;
	item->next = item;
	item->container = container;
	item->list = NULL;
	item->value = value;
}

/**
 * @brief Inserts a prioritized list item to a prioritized list
 * @param item pointer to the item to be inserted to the list
 * @param list pointer to the list
 * @details The list allows multiple items to have the same value.
 * The items with the smaller values will always be inserted to the front
 * of the list, with the @ref prioritizedList.first pointer pointing to
 * the item with the smallest value. For items with the same values, the
 * they will be arranged in the order they are inserted.
 */
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
		listItemCookie_insertBefore( item, list->first );
	}
	else if( item->value < list->first->value )
	{
		/* insert as first item */
		listItemCookie_insertBefore( item, list->first );
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

		listItemCookie_insertBefore( item, i);
	}

	item->list = list;
}

/**
 * @brief Removes a prioritized or not prioritized list item from its list
 * @param p pointer to the item to be removed
 */
void
list_remove( void* p )
{
	NotPrioritizedListItem_t* item = (NotPrioritizedListItem_t*)p;

	OS_ASSERT( item->list != NULL );

	if( item == item->list->first )
	{
		/* 'first' should always point into the list */
		item->list->first = item->list->first->next;

		if( item == item->list->first )
			/* this means that this item is the only item in the list */
			item->list->first = NULL;
	}

	listItemCookie_remove( item );
	item->list = NULL;
}
