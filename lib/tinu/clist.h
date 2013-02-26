/* TINU - Unittesting framework
*
* Copyright (c) 2010, Viktor Hercinger <hercinger.viktor@gmail.com>
* All rights reserved.
* 
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*     * Redistributions of source code must retain the above copyright
*       notice, this list of conditions and the following disclaimer.
*     * Redistributions in binary form must reproduce the above copyright
*       notice, this list of conditions and the following disclaimer in the
*       documentation and/or other materials provided with the distribution.
*     * Neither the name of the original author (Viktor Hercinger) nor the
*       names of its contributors may be used to endorse or promote products
*       derived from this software without specific prior written permission.
* 
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
* ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER BE LIABLE FOR ANY
* DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
* LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
* ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
* SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*
* Author(s): Viktor Hercinger <hercinger.viktor@gmail.com>
*/

/** @file clist.h
 * @brief Circularly linked list.
 *
 * This is an implementation of a circularly linked list. It should be used
 * when appending/prepending should be O(1) and no search on the items is
 * required (i.e. a list of hooks in which each one is to be called). CList
 * uses the GLib slice allocator for better performance.
 *
 * It should be noted that all items in the list are valid list heads, so the
 * list can be 'rotated' that is the list head can be changed to the next or
 * previous item.
 */
#ifndef _TINU_CLIST_H
#define _TINU_CLIST_H

#include <glib.h>

__BEGIN_DECLS

/** @brief Circularly linked list
 *
 * This structure contains the linked list head and items. Since the list is circular,
 * the following is true:
 * a., if the list head is NULL, the list is empty. NULL is a valid list.
 * b., if the list has only one item, the next and previous pointers will point
 *     to the list heaad
 * c., if the list has more than one elements, the next/prev pointers will point
 *     to the appropriate item.
 */
typedef struct _CList
{
  /** List data, has to be freed by the user */
  gpointer            m_data;

  /** Next item in list */
  struct _CList      *m_next;
  /** Previous item in list */
  struct _CList      *m_prev;
} CList;

/** @brief List iterator
 *
 * This iterator should be used to iterate through the list. Iterating through
 * this list should look something like this:
 * @code
 * CList *list;
 * CListIterator *iter;
 * 
 * // ...
 *
 * for (iter = clist_iter_new(list); clist_iter_next(iter); )
 *   {
 *     gpointer data = clist_iter_data(iter);
 *     // actual code
 *   }
 * clist_iter_done(iter);
 * @endcode
 *
 * The clist_iter_new function creates an iterator of the list. The clist_iter_next
 * iterates through the list and returns FALSE if the iterator reaches the item
 * it started with (i.e. the CList item that was used to create the iterator).
 */
typedef struct _CListIterator CListIterator;

/** @brief Callback for clist_foreach.
 * @param data Data in list entry.
 * @param user_data User data supplied by the caller
 * @return Should return FALSE if clist_foreach should terminate after the call.
 *
 * This callback is called with the list data and a custom user data for
 * each item in the list
 */
typedef gboolean (*CListForeachCb)(gpointer data, gpointer user_data);
/** @brief Callback for destroying the list data
 * @param data Data in list entry.
 *
 * This called is called when the list is being destroyed to free the
 * list data.
 */
typedef void (CListDataDestroyCb)(gpointer data);

/** @brief Destroy the list
 * @param self Arbitary item in the list. May be NULL, then this does nothing.
 * @param destroy Callback to destroy the list data with. May be NULL.
 *
 * Destroy the list and call destroy (if suplied) on each item in the list.
 * If the destroy callback is NULL, it is not called.
 */
void clist_destroy(CList *self, CListDataDestroyCb destroy);

/** @brief Append data to the list
 * @param self Arbitary item in the list. May be NULL.
 * @param data Data to append.
 * @return self or if self was NULL then a new list.
 *
 * Appends a new item to the "end" of the list (or if self is NULL a new
 * list is created with one item). In this context, end means before self,
 * so when iterating through the list this item will appear as the last one.
 */
CList *clist_append(CList *self, gpointer data);

/** @brief Prepend data to the list
 * @param self Arbitary item in the list. May be NULL.
 * @param data Data to append.
 * @return self or if self was NULL then a new list.
 *
 * Inserts a new item to the "beginning" of the list (or if self is NULL a new
 * list is created with one item). In this context, beginning means after self,
 * so when iterating through the list this item will appear as the first one.
 */
CList *clist_prepend(CList *self, gpointer data);

/** @brief Remove item from the list
 * @param self Arbitary item in the list. May be NULL, then this does nothing.
 * @param destroy Callback to destroy the list data with. May be NULL.
 * @return The item that came after self or NULL if the list had one item
 *
 * Removes the current element (calls destroy on data if suplied) and returns
 * the next item in list (the one that came after self).
 */
CList *clist_remove(CList *self, CListDataDestroyCb destroy);

/** @brief Iterate throught the list
 * @param self Arbitary item in the list. May be NULL.
 * @param list_cb Callback
 * @param user_data User data to pass to list_cb
 * @return TRUE if the list iterated through each item.
 */
gboolean clist_foreach(CList *self, CListForeachCb list_cb, gpointer user_data);

/** @brief Create an iterator
 * @param list List to iterate through.
 * @return A new iterator object
 *
 * @note Parameter list may be NULL, then the list is considered empty. In this
 * case the iterator will return FALSE with the first call to clist_iter_next and
 * clist_iter_prev.
 * @note This struct should be freed with clist_iter_done
 */
CListIterator *clist_iter_new(CList *list);
/** @brief Free iterator
 * @param iter Iterator
 */
void clist_iter_done(CListIterator *iter);
/** @brief Check if iterator is valid
 * @param iter Iterator
 * @return TRUE if iterator is valid
 */
gboolean clist_iter_valid(CListIterator *iter);
/** @brief Step to the next item in iterator
 * @param iter Iterator
 * @return TRUE if iterator is valid after the call
 *
 * @note Should not be mixed with calls to clist_iter_prev.
 */
gboolean clist_iter_next(CListIterator *iter);
/** @brief Step to the previous item in iterator
 * @param iter Iterator
 * @return TRUE if iterator is valid after the call
 *
 * @note Should not be mixed with calls to clist_iter_next.
 */
gboolean clist_iter_prev(CListIterator *iter);
/** @brief Get the data at the iterator
 * @param iter Iterator
 * @return The iterator data.
 *
 * @note This call fails with an assertion if the iterator is invalid
 * or clist_iter_next or clist_iter_prev was not called prior to the call.
 */
gpointer clist_iter_data(CListIterator *iter);
/** @brief Remove item at iterator
 * @param iter Iterator
 * @param destroy Destroy callback (like in clist_remove)
 */
void clist_iter_remove(CListIterator *iter, CListDataDestroyCb destroy);

__END_DECLS

#endif
