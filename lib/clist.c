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

#include <stdio.h>
#include <tinu/clist.h>

#include <glib/gmem.h>
#include <glib/gtestutils.h>

struct _CListIterator
{
  CList              *m_start;
  CList              *m_act;
};

void
clist_destroy(CList *self, CListDataDestroyCb destroy)
{
  CList *act, *next;

  if (!self)
    return;

  act = self;
  next = self->m_next;
  do
    {
      next = act->m_next;
      if (destroy)
        destroy(act->m_data);
      g_slice_free(CList, act);
    } while (act != self);
}

CList *
clist_append(CList *self, gpointer data)
{
  CList *new_item = g_slice_new0(CList);

  new_item->m_data = data;
  if (!self)
    {
      new_item->m_next = new_item;
      new_item->m_prev = new_item;
      return new_item;
    }

  new_item->m_next = self;
  new_item->m_prev = self->m_prev;
  self->m_prev->m_next = new_item;
  self->m_prev = new_item;
  return self;
}

CList *
clist_prepend(CList *self, gpointer data)
{
  CList *nlist = clist_append(self, data);
  return nlist->m_prev;
}

CList *
clist_remove(CList *self, CListDataDestroyCb destroy)
{
  CList *res;

  if (!self)
    return NULL;

  if (self->m_next == self)
    {
      /* Last item */
      res = NULL;
    }
  else
    {
      /* There are further items */
      self->m_next->m_prev = self->m_prev;
      self->m_prev->m_next = self->m_next;
      res = self->m_next;
    }

  if (destroy)
    destroy(self->m_data);
  g_slice_free(CList, self);
  return res;
}

gboolean
clist_foreach(CList *self, CListForeachCb list, gpointer user_data)
{
  CList *act;

  if (!self)
    return TRUE;

  act = self;
  do
    {
      if (!list(act->m_data, user_data))
        return FALSE;
      act = act->m_next;
    } while (act != self);
  return TRUE;
}

CListIterator *
clist_iter_new(CList *list)
{
  CListIterator *res = g_new0(CListIterator, 1);
  res->m_start = list;
  res->m_act = NULL;
  return res;
}

void
clist_iter_done(CListIterator *iter)
{
  g_free(iter);
}

gboolean
clist_iter_valid(CListIterator *iter)
{
  return iter->m_start && (!iter->m_act || iter->m_act != iter->m_start);
}

gboolean
clist_iter_next(CListIterator *iter)
{
  if (!iter->m_start)
    return FALSE;

  if (!iter->m_act)
    {
      iter->m_act = iter->m_start;
      return TRUE;
    }

  g_assert (iter->m_act->m_next != NULL);

  iter->m_act = iter->m_act->m_next;
  return iter->m_act != iter->m_start;
}

gboolean
clist_iter_prev(CListIterator *iter)
{
  if (!iter->m_start)
    return FALSE;

  if (!iter->m_act)
    {
      iter->m_act = iter->m_start;
      return TRUE;
    }

  g_assert (iter->m_act->m_prev != NULL);

  iter->m_act = iter->m_act->m_prev;
  return iter->m_act != iter->m_start;
}

gpointer
clist_iter_data(CListIterator *iter)
{
  g_assert (iter->m_act);
  return iter->m_act->m_data;
}

void
clist_iter_remove(CListIterator *iter, CListDataDestroyCb destroy)
{
  gboolean rebase;

  g_assert (iter->m_act);
  rebase = (iter->m_act == iter->m_start);
  iter->m_act = clist_remove(iter->m_act, destroy);

  if (rebase)
    iter->m_start = iter->m_act;
}
