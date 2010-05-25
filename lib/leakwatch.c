/* TINU - Unittesting framework
*
* Copyright (c) 2009, Viktor Hercinger <hercinger.viktor@gmail.com>
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
#include <stdlib.h>
#include <string.h>
#include <malloc.h>

#include <glib/gtestutils.h>
#include <glib/gthread.h>
#include <glib/ghash.h>
#include <glib/glist.h>

#include <tinu/utils.h>
#include <tinu/leakwatch.h>

static GSList *g_leakwatch_list = NULL;
static gint g_leakwatch_count = 0;
static gboolean g_leakwatch_init = FALSE;

static void *(*g_leakwatch_malloc)(size_t, const void *) = NULL;
static void *(*g_leakwatch_realloc)(void *, size_t, const void *) = NULL;
static void  (*g_leakwatch_free)(void *, const void *) = NULL;

struct _Leakwatch
{
  gpointer        m_user_data;
  AllocCallback   m_callback;
};

void _hook_pause();
void _hook_resume();

void
_leakwatch_alert(LeakwatchOperation op, gpointer oldptr, gpointer ptr, gsize size)
{
  struct _Leakwatch *cb;
  Backtrace *trace = backtrace_create(4);
  GSList *act;

#ifdef TRACK_ALERTS
  fprintf(stderr, "[track] op=%d old=%p res=%p size=%d\n", op, oldptr, ptr, size);
#endif

  for (act = g_leakwatch_list; act; act = act->next)
    {
      cb = (struct _Leakwatch *)act->data;
      cb->m_callback(op, oldptr, ptr, size, trace, cb->m_user_data);
    }

  backtrace_unreference(trace);
}

void *
_leakwatch_malloc(size_t size, const void *caller)
{
  void *res;

  _hook_pause();

  res = g_leakwatch_malloc ? g_leakwatch_malloc(size, caller) : malloc(size);
  _leakwatch_alert(LEAKWATCH_OPERATION_MALLOC, NULL, res, size);

  _hook_resume();
  return res;
}

void *
_leakwatch_realloc(void *ptr, size_t newsize, const void *caller)
{
  void *res;

  _hook_pause();

  res = g_leakwatch_realloc ? g_leakwatch_realloc(ptr, newsize, caller) : realloc(ptr, newsize);
  _leakwatch_alert(LEAKWATCH_OPERATION_REALLOC, ptr, res, newsize);

  _hook_resume();
  return res;
}

void
_leakwatch_free(void *ptr, const void *caller)
{
  _hook_pause();

  _leakwatch_alert(LEAKWATCH_OPERATION_FREE, ptr, NULL, 0);

  if (g_leakwatch_free)
    g_leakwatch_free(ptr, caller);
  else
    free(ptr);

  _hook_resume();
}

void
_hook_pause()
{
  if (g_leakwatch_count > 0)
    {
      __malloc_hook = NULL;
      __realloc_hook = NULL;
      __free_hook = NULL;
    }
}

void
_hook_resume()
{
  if (g_leakwatch_count > 0)
    {
      __malloc_hook = _leakwatch_malloc;
      __realloc_hook = _leakwatch_realloc;
      __free_hook = _leakwatch_free;
    }
}

void
_memory_entry_destroy(gpointer obj)
{
  MemoryEntry *self = (MemoryEntry *)obj;

  backtrace_unreference(self->m_origin);
  backtrace_unreference(self->m_last);
  t_free(obj);
}

void
_tinu_leakwatch_simple_callback(LeakwatchOperation operation,
  gpointer oldptr, gpointer ptr, gsize size,
  Backtrace *trace,
  gpointer user_data)
{
  GHashTable *self = g_hash_table_ref((GHashTable *)user_data);
  MemoryEntry *entry, *oldentry;

  switch (operation)
    {
      case LEAKWATCH_OPERATION_MALLOC :
        entry = t_new(MemoryEntry, 1);

        entry->m_ptr = ptr;
        entry->m_size = size;
        entry->m_origin = backtrace_reference(trace);

        g_hash_table_insert(self, ptr, entry);
        break;

      case LEAKWATCH_OPERATION_REALLOC :
        oldentry = (MemoryEntry *)g_hash_table_lookup(self, oldptr);

        if (!oldentry)
          break;

        entry = t_new(MemoryEntry, 1);

        entry->m_ptr = ptr;
        entry->m_size = size;
        entry->m_origin = (oldentry ? backtrace_reference(oldentry->m_origin) : NULL);
        entry->m_last = backtrace_reference(trace);

        g_hash_table_replace(self, ptr, entry);
        break;

      case LEAKWATCH_OPERATION_FREE :
        g_hash_table_remove(self, oldptr);
        break;

      default :
        g_assert_not_reached();
    }

  g_hash_table_unref(self);
}

void
_tinu_leakwatch_simple_dump(gpointer key, gpointer value, gpointer user_data)
{
  gint priority = (gint)user_data;
  MemoryEntry *self = (MemoryEntry *)value;

  log_format(priority, "Memory leak found",
            msg_tag_ptr("pointer", self->m_ptr),
            msg_tag_int("size", self->m_size), NULL);

  if (self->m_origin)
    {
      log_format(priority, "  Original allocator", NULL);
      backtrace_dump_log(self->m_origin, "    ", priority);
    }

  if (self->m_last)
    {
      log_format(priority, "  Last (re)allocator", NULL);
      backtrace_dump_log(self->m_last, "    ", priority);
    }
}

void
_tinu_leakwatch_enable()
{
  if (g_leakwatch_count == 0)
    {
      g_leakwatch_malloc = __malloc_hook;
      g_leakwatch_realloc = __realloc_hook;
      g_leakwatch_free = __free_hook;

      __malloc_hook = _leakwatch_malloc;
      __realloc_hook = _leakwatch_realloc;
      __free_hook = _leakwatch_free;
    }
  g_leakwatch_count++;
}

static void
_tinu_leakwatch_clear()
{
  __malloc_hook = g_leakwatch_malloc;
  __realloc_hook = g_leakwatch_realloc;
  __free_hook = g_leakwatch_free;

  g_leakwatch_malloc = NULL;
  g_leakwatch_realloc = NULL;
  g_leakwatch_free = NULL;
}

void
_tinu_leakwatch_disable()
{
  t_assert(g_leakwatch_count > 0);
  if (g_leakwatch_count == 1)
    {
      _tinu_leakwatch_clear(NULL);
      t_assert(!g_leakwatch_list);
    }
  g_leakwatch_count--;
}

gpointer
tinu_register_watch(AllocCallback callback, gpointer user_data)
{
  struct _Leakwatch *lw;

  if (!g_leakwatch_init)
    {
      atexit(_tinu_leakwatch_clear);
      g_leakwatch_init = TRUE;
    }

  _tinu_leakwatch_enable();
  _hook_pause();

  lw = t_new(struct _Leakwatch, 1);
  lw->m_user_data = user_data;
  lw->m_callback = callback;

  g_leakwatch_list = g_slist_prepend(g_leakwatch_list, lw);

  _hook_resume();
  return (gpointer)lw;
}

gboolean
tinu_unregister_watch(gpointer handle)
{
  GSList *act;
  struct _Leakwatch *lw;
  gboolean res = FALSE;

  if (!g_leakwatch_init)
    {
      log_emerg("Leakwatch subsystem uninitialized", NULL);
      return FALSE;
    }

  _hook_pause();

  for (act = g_leakwatch_list; act; act = act->next)
    {
      lw = (struct _Leakwatch *)act->data;

      if (act->data == handle)
        {
          res = TRUE;
          t_free(lw);
          act->data = NULL;
          g_leakwatch_list = g_slist_delete_link(g_leakwatch_list, act);
          break;
        }
    }

  _hook_resume();
  if (act)
    _tinu_leakwatch_disable();
  else
    log_warn("Leakwatch handle is not registered",
             msg_tag_ptr("handle", handle), NULL);

  return res;
}

gpointer
tinu_leakwatch_simple(GHashTable **result)
{
  t_assert(result);
  t_assert(*result == NULL);

  *result = g_hash_table_new_full(g_direct_hash, g_direct_equal, NULL, _memory_entry_destroy);
  return tinu_register_watch(_tinu_leakwatch_simple_callback, (gpointer)*result);
}

void
tinu_leakwatch_simple_dump(GHashTable *result, gint loglevel)
{
  g_hash_table_foreach(result, _tinu_leakwatch_simple_dump, (gpointer)loglevel);
}

