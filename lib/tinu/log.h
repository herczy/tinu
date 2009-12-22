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
