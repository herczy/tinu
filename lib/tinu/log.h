#ifndef _TINU_LOG_H
#define _TINU_LOG_H

#include <stdio.h>
#include <stdarg.h>

#include <glib/gtypes.h>

#include <tinu/message.h>

typedef gboolean (*MessageHandler)(Message *msg, gpointer user_data);

/* gchar *msg_string(Message *self, gchar *buffer, gint maxlen, gboolean add_priority); */

#define LOGMSG_NO_PROPAGATE   ((gpointer)FALSE)
#define LOGMSG_PROPAGATE      ((gpointer)TRUE)
gboolean msg_syslog_handler(Message *msg, gpointer user_data);
gboolean msg_stderr_handler(Message *msg, gpointer user_data);
gboolean msg_stderr_fancy_handler(Message *msg, gpointer user_data);
gboolean msg_fail_handler(Message *msg, gpointer user_data);
gboolean msg_file_handler(Message *msg, gpointer user_data);

void tinu_log(Message *msg, gboolean free_msg);
void tinu_plog(gint priority, const gchar *msg, MessageTag *tag0, ...);
void tinu_vplog(gint priority, const gchar *msg, MessageTag *tag0, va_list vl);

void tinu_log_init();
void tinu_log_clear();
/* Messages will be send to given handler. Cannot be reinitialized */
void tinu_log_divert(MessageHandler handler, gpointer user_data);

gint tinu_log_max_priority();

// Fatal error; no chance of recovery or continuation
#define log_abort(msg...)   tinu_plog(LOG_CRIT, msg)
#define log_error(msg...)   tinu_plog(LOG_ERR, msg)
#define log_warn(msg...)    tinu_plog(LOG_WARNING, msg)
#define log_notice(msg...)  tinu_plog(LOG_NOTICE, msg)
#define log_info(msg...)    tinu_plog(LOG_INFO, msg)
#define log_debug(msg...)   tinu_plog(LOG_DEBUG, msg)

#define t_assert(cond)                                                  \
  if (!(cond))                                                          \
    {                                                                   \
      log_abort("Assertion failed",                                     \
                msg_tag_str("condition", # cond),                       \
                msg_tag_str("function", __PRETTY_FUNCTION__),           \
                msg_tag_str("filename", __FILE__),                      \
                msg_tag_int("line", __LINE__),                          \
                msg_tag_trace_current("trace", 3), NULL);               \
      abort();                                                          \
    }

#define t_raw_assert(cond)                                              \
  if (!(cond))                                                          \
    {                                                                   \
      fprintf(stderr, "Raw assert `" # cond "` failed (%s:%d (%s))\n",  \
              __FILE__, __LINE__, __PRETTY_FUNCTION__);                 \
      abort();                                                          \
    }

gpointer tinu_register_message_handler(MessageHandler handler,
  gint max_priority, gpointer user_data);
void tinu_unregister_message_handler(gpointer handler);

#endif
