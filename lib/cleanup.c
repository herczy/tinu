#include <stdlib.h>

#include <glib/glist.h>
#include <glib/gslist.h>

#include <tinu/utils.h>
#include <tinu/cleanup.h>

static gboolean g_cleanup_init = FALSE;
static GSList *g_cleanup_list = NULL;

typedef struct _CleanupAction
{
  CleanupFunc         m_cleanup;
  gpointer            m_user_data;
} CleanupAction;

void
_run_cleanup()
{
  GSList *act;
  CleanupAction *action;

  for (act = g_cleanup_list; act; act = act->next)
    {
      action = (CleanupAction *)act->data;
      action->m_cleanup(action->m_user_data);
      t_free(action);
    }

  g_slist_free(g_cleanup_list);
  g_cleanup_list = NULL;
}

gpointer
tinu_atexit(CleanupFunc cleanup, gpointer user_data)
{
  CleanupAction *res = t_new(CleanupAction, 1);

  res->m_cleanup = cleanup;
  res->m_user_data = user_data;

  if (!g_cleanup_init)
    {
      atexit(_run_cleanup);
      g_cleanup_init = TRUE;
    }

  g_cleanup_list = g_slist_prepend(g_cleanup_list, res);
  return (gpointer)res;
}

void
tinu_atexit_clear(gpointer handle)
{
  GSList *item;

  if (!g_cleanup_init)
    return;

  item = g_slist_find(g_cleanup_list, handle);

  if (!item)
    return;

  t_free(item);
  g_cleanup_list = g_slist_delete_link(g_cleanup_list, item);
}

void
tinu_reset()
{
  _run_cleanup();
}

