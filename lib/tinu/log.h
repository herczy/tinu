#ifndef _TINU_LOG_H
#define _TINU_LOG_H

#include <stdio.h>
#include <stdarg.h>

#include <glib/gtypes.h>

#include <tinu/message.h>

typedef gboolean (*MessageHandler)(Message *msg, gpointer user_data);

#define LOGMSG_NO_PROPAGATE   ((gpointer)FALSE)
#define LOGMSG_PROPAGATE      ((gpointer)TRUE)

gboolean msg_syslog_handler(Message *msg, gpointer user_data);
gboolean msg_stderr_handler(Message *msg, gpointer user_data);
gboolean msg_stderr_fancy_handler(Message *msg, gpointer user_data);
gboolean msg_fail_handler(Message *msg, gpointer user_data);

gboolean msg_file_handler(Message *msg, gpointer user_data);

void log_message(Message *msg, gboolean free_msg);
void log_format(gint priority, const gchar *msg, MessageTag *tag0, ...);
void log_vformat(gint priority, const gchar *msg, MessageTag *tag0, va_list vl);

void log_init();
void log_clear();

/* Messages will be send to given handler. Cannot be reinitialized after this */
void log_divert(MessageHandler handler, gpointer user_data);

#define log_wrap(pri, msg...) \
  do { if (g_log_max_priority >= (pri)) { log_format((pri), msg); } } while (0)

#define log_emerg(msg...)   log_wrap(LOG_EMERG, msg)
#define log_alert(msg...)   log_wrap(LOG_ALERT, msg)
#define log_crit(msg...)    log_wrap(LOG_CRIT, msg)
#define log_error(msg...)   log_wrap(LOG_ERR, msg)
#define log_warn(msg...)    log_wrap(LOG_WARNING, msg)
#define log_notice(msg...)  log_wrap(LOG_NOTICE, msg)
#define log_info(msg...)    log_wrap(LOG_INFO, msg)
#define log_debug(msg...)   log_wrap(LOG_DEBUG, msg)

extern gint g_log_max_priority;

gpointer log_register_message_handler(MessageHandler handler,
  gint max_priority, gpointer user_data);
void log_unregister_message_handler(gpointer handler);

#endif
