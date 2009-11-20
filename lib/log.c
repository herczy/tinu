#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <errno.h>

#include <glib/glist.h>
#include <glib/gslist.h>
#include <glib/gmessages.h>

#include <tinu/log.h>

static GSList *g_log_list = NULL;
gint g_log_max_priority = -1;

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

static void
_log_alert_handlers(Message *msg)
{
  GSList *act;
  struct _LogHandler *self;
  gboolean propagate = TRUE;

  for (act = g_log_list; act && propagate; act = act->next)
    {
      self = (struct _LogHandler *)act->data;

      if (self->m_max_priority >= msg->m_priority)
        propagate = self->m_handler(msg, self->m_user_data);
    }
}

gpointer
log_register_message_handler(MessageHandler handler, gint max_priority, gpointer user_data)
{
  struct _LogHandler *self;

  /* If messages are diverted, don't accept handlers */
  if (g_log_diverted.m_divert_handler)
    return NULL;

  self = g_new0(struct _LogHandler, 1);
  self->m_max_priority = max_priority;
  self->m_user_data = user_data;
  self->m_handler = handler;

  if (max_priority > g_log_max_priority)
    g_log_max_priority = max_priority;

  g_log_list = g_slist_prepend(g_log_list, self);

  return (gpointer)self;
}

void
log_unregister_message_handler(gpointer handler)
{
  GSList *act, *next;

  /* If messages are diverted, this is not necessary */
  if (g_log_diverted.m_divert_handler)
    return;

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
}

gboolean
msg_syslog_handler(Message *msg, gpointer user_data)
{
  gchar *buffer = msg_format_simple(msg);
  syslog(msg->m_priority, "%s", buffer);
  g_free(buffer);
  return (gboolean)user_data;
}

gboolean
msg_stderr_handler(Message *msg, gpointer user_data)
{
  msg_file_handler(msg, (gpointer)stderr);
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
  if (msg->m_priority <= (gint)user_data)
    abort();
  return TRUE;
}

gboolean
msg_file_handler(Message *msg, gpointer user_data)
{
  gchar *buffer = msg_format_simple(msg);
  fprintf((FILE *)user_data, "[%s] %s\n", msg_format_priority(msg->m_priority), buffer);
  g_free(buffer);
  return TRUE;
}

void
log_message(Message *msg, gboolean free_msg)
{
  if (g_log_diverted.m_divert_handler)
    {
      g_log_diverted.m_divert_handler(msg, g_log_diverted.m_user_data);
      goto exit;
    }

  if (!g_log_list || msg->m_priority > g_log_max_priority)
    goto exit;

  _log_alert_handlers(msg);

exit:
  if (free_msg)
    msg_destroy(msg);
}

void
log_vformat(gint priority, const gchar *msg, MessageTag *tag0, va_list vl)
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
          g_free(tag->m_tag);
          g_free(tag->m_value);
          g_free(tag);
          tag = va_arg(vl, MessageTag *);
        }
      return;
    }

  self = msg_vcreate(priority, msg, tag0, vl);
  _log_alert_handlers(self);
  msg_destroy(self);
}

void
log_format(gint priority, const gchar *msg, MessageTag *tag0, ...)
{
  va_list vl;

  va_start(vl, tag0);
  log_vformat(priority, msg, tag0, vl);
  va_end(vl);
}

void
log_init()
{
}

void
log_clear()
{
  GSList *act;

  g_log_max_priority = -1;

  for (act = g_log_list; act; act = act->next)
    {
      g_free(act->data);
    }
  g_slist_free(g_log_list);
  g_log_list = NULL;
}

void
log_divert(MessageHandler handler, gpointer user_data)
{
  log_clear();

  g_log_diverted.m_divert_handler = handler;
  g_log_diverted.m_user_data = user_data;
  g_log_max_priority = LOG_DEBUG;
}
