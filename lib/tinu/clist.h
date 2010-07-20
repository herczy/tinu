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

#ifndef _TINU_CLIST_H
#define _TINU_CLIST_H

#include <glib/gtypes.h>

__BEGIN_DECLS

typedef struct _CList
{
  gpointer            m_data;

  struct _CList      *m_next;
  struct _CList      *m_prev;
} CList;

typedef struct _CListIterator CListIterator;

typedef gboolean (*CListForeachCb)(gpointer data, gpointer user_data);
typedef void (CListDataDestroyCb)(gpointer data);

void clist_destroy(CList *self, CListDataDestroyCb destroy);

CList *clist_append(CList *self, gpointer data);
CList *clist_prepend(CList *self, gpointer data);
CList *clist_remove(CList *self, CListDataDestroyCb destroy);

gboolean clist_foreach(CList *self, CListForeachCb list_cb, gpointer user_data);

CListIterator *clist_iter_new(CList *list);
void clist_iter_done(CListIterator *iter);
gboolean clist_iter_valid(CListIterator *iter);
gboolean clist_iter_next(CListIterator *iter);
gboolean clist_iter_prev(CListIterator *iter);
gpointer clist_iter_data(CListIterator *iter);
void clist_iter_remove(CListIterator *iter, CListDataDestroyCb destroy);

__END_DECLS

#endif
