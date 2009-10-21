#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <errno.h>

#include <glib/glist.h>
#include <glib/gslist.h>
#include <glib/gmessages.h>

#include <tinu/cleanup.h>
#include <tinu/backtrace.h>
#include <tinu/utils.h>
#include <tinu/log.h>

#ifdef ENABLE_THREADS
#define _lock   g_static_mutex_lock(&g_log_lock)
#define _unlock g_static_mutex_unlock(&g_log_lock)

static GStaticMutex g_log_lock = G_STATIC_MUTEX_INIT;
#else
#define _lock
#define _unlock
#endif

static GSList *g_log_list = NULL;
gint g_log_max_priority = -1;
static gpointer g_log_cleanup_handle = NULL;

static struct 
{
  MessageHandler    m_divert_handler;
  gpointer          m_user_data;
} g_log_diverted = { NULL, NULL };

struct _LogHandler
{
  gint            m_max_priority;
  gpointer        m_user_data;
  MessageHandler  m_handler;
};

void
_tinu_glog_handler(const gchar *domain, GLogLevelFlags flags,
                   const gchar *msg, gpointer user_data G_GNUC_UNUSED)
{
  gint priority;
  switch (flags & G_LOG_LEVEL_MASK)
    {
      case G_LOG_LEVEL_ERROR :
        priority = LOG_CRIT;
        break;

      case G_LOG_LEVEL_CRITICAL :
        priority = LOG_ERR;
        break;

      case G_LOG_LEVEL_WARNING :
        priority = LOG_WARNING;
        break;

      case G_LOG_LEVEL_MESSAGE :
        priority = LOG_NOTICE;
        break;

      case G_LOG_LEVEL_INFO :
        priority = LOG_INFO;
        break;

      case G_LOG_LEVEL_DEBUG :
        priority = LOG_DEBUG;
        break;

      default :
        priority = LOG_DEBUG;
        break;
    }

  tinu_plog(priority, msg, msg_tag_str("domain", domain), msg_tag_trace_current("trace", 6), NULL);
}

static void
_tinu_alert_log_handlers(Message *msg)
{
  GSList *act;
  struct _LogHandler *self;
  gboolean propagate = TRUE;

  _lock;

  for (act = g_log_list; act && propagate; act = act->next)
    {
      self = (struct _LogHandler *)act->data;

      if (self->m_max_priority >= msg->m_priority)
        propagate = self->m_handler(msg, self->m_user_data);
    }

  _unlock;
}

static void
_tinu_log_cleanup(gpointer user_data G_GNUC_UNUSED)
{
  GSList *act;

  _lock;
  g_log_max_priority = -1;

  for (act = g_log_list; act; act = act->next)
    {
      t_free(act->data);
    }
  g_slist_free(g_log_list);
  g_log_list = NULL;

  g_log_cleanup_handle = NULL;

  _unlock;
}

gpointer
tinu_register_message_handler(MessageHandler handler, gint max_priority, gpointer user_data)
{
  struct _LogHandler *self;

  /* If messages are diverted, don't accept handlers */
  if (g_log_diverted.m_divert_handler)
    return NULL;

  tinu_log_init();

  _lock;
  self = t_new(struct _LogHandler, 1);
  self->m_max_priority = max_priority;
  self->m_user_data = user_data;
  self->m_handler = handler;

  if (max_priority > g_log_max_priority)
    g_log_max_priority = max_priority;

  g_log_list = g_slist_prepend(g_log_list, self);

  _unlock;
  return (gpointer)self;
}

void
tinu_unregister_message_handler(gpointer handler)
{
  GSList *act, *next;

  /* If messages are diverted, this is not necessary */
  if (g_log_diverted.m_divert_handler)
    return;

  _lock;
  g_log_max_priority = LOG_EMERG;
  for (act = g_log_list; act; act = next)
    {
      next = act->next;
      if (act->data == handler)
        {
          free(act->data);
          g_log_list = g_slist_delete_link(g_log_list, act);
        }
      else
        {
          if (((struct _LogHandler *)act->data)->m_max_priority > g_log_max_priority)
            g_log_max_priority = ((struct _LogHandler *)act->data)->m_max_priority;
        }
    }

  if (!g_log_list)
    g_log_max_priority = -1;

  _unlock;
}

gboolean
msg_syslog_handler(Message *msg, gpointer user_data)
{
  gchar *buffer = msg_format_simple(msg);
  syslog(msg->m_priority, "%s", buffer);
  t_free(buffer);
  return (gboolean)user_data;
}

gboolean
msg_stderr_handler(Message *msg, gpointer user_data)
{
  gchar *buffer = msg_format_simple(msg);
  fprintf(stderr, "[%s] %s\n", msg_format_priority(msg->m_priority), buffer);
  t_free(buffer);
  return (gboolean)user_data;
}

gboolean
msg_stderr_fancy_handler(Message *msg, gpointer user_data)
{
  const gchar *color;
  gint i;

  switch (msg->m_priority)
    {
      case LOG_EMERG :
      case LOG_ALERT :
      case LOG_CRIT :
        color = "\033[41m\033[1;37m";
        break;

      case LOG_ERR :
        color = "\033[1;31m";
        break;

      case LOG_WARNING :
        color = "\033[1;33m";
        break;

      case LOG_DEBUG :
        color = "\033[1;30m";
        break;

      default :
        color = "\033[1;34m";
        break;
    }

  fprintf(stderr, "[%s%s\033[0m] %s", color, msg_format_priority(msg->m_priority), msg->m_message);

  for (i = 0; i < msg->m_tag_count; i++)
    {
      fprintf(stderr, " [\033[36m%s\033[0m=%s]",
                      msg->m_tags[i]->m_tag,
                      msg->m_tags[i]->m_value);
    }

  fprintf(stderr, "\n");
  return (gboolean)user_data;
}

gboolean
msg_fail_handler(Message *msg, gpointer user_data)
{
  Backtrace *trace;
  if (msg->m_priority <= (gint)user_data)
    {
      trace = backtrace_create(4);

      msg_stderr_handler(msg, NULL);
      fprintf(stderr, "Backtrace:\n");
      backtrace_dump_file(trace, stderr, 4);
      fprintf(stderr, "\n");

      abort();
    }
  return TRUE;
}

void
tinu_log(Message *msg, gboolean free_msg)
{
  if (g_log_diverted.m_divert_handler)
    {
      g_log_diverted.m_divert_handler(msg, g_log_diverted.m_user_data);
      goto exit;
    }

  if (!g_log_list || msg->m_priority > g_log_max_priority)
    goto exit;

  _tinu_alert_log_handlers(msg);

exit:
  if (free_msg)
    msg_destroy(msg);
}

void
tinu_vplog(gint priority, const gchar *msg, MessageTag *tag0, va_list vl)
{
  Message *self;
  MessageTag *tag;

  if (g_log_diverted.m_divert_handler)
    {
      self = msg_vcreate(priority, msg, tag0, vl);
      g_log_diverted.m_divert_handler(self, g_log_diverted.m_user_data);
      msg_destroy(self);
      return;
    }

  if (!g_log_list || priority > g_log_max_priority)
    {
      tag = tag0;
      while (tag)
        {
          t_free(tag->m_tag);
          t_free(tag->m_value);
          t_free(tag);
          tag = va_arg(vl, MessageTag *);
        }
      return;
    }

  self = msg_vcreate(priority, msg, tag0, vl);
  _tinu_alert_log_handlers(self);
  msg_destroy(self);
}

void
tinu_plog(gint priority, const gchar *msg, MessageTag *tag0, ...)
{
  va_list vl;

  va_start(vl, tag0);
  tinu_vplog(priority, msg, tag0, vl);
  va_end(vl);
}

void
tinu_log_init()
{
  _lock;
  if (!g_log_cleanup_handle)
    {
      g_log_cleanup_handle = tinu_atexit(_tinu_log_cleanup, NULL);
      g_log_set_default_handler(_tinu_glog_handler, NULL);
    }
  _unlock;
}

void
tinu_log_clear()
{
  _tinu_log_cleanup(NULL);
}

void
tinu_log_divert(MessageHandler handler, gpointer user_data)
{
  _tinu_log_cleanup(NULL);

  _lock;
  g_log_diverted.m_divert_handler = handler;
  g_log_diverted.m_user_data = user_data;
  g_log_max_priority = LOG_DEBUG;
  _unlock;
}

gint
tinu_log_max_priority()
{
  return g_log_max_priority;
}

