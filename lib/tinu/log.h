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

/** @file log.h
 * @brief Message logging
 *
 * This file contains the logging facilities used by tinu.
 */
#ifndef _TINU_LOG_H
#define _TINU_LOG_H

#include <stdio.h>
#include <stdarg.h>

#include <glib/gtypes.h>

#include <tinu/message.h>

/** @brief Message handler
 * @param msg Message to be handled
 * @param user_data User data defined when adding handler
 * @return Should return TRUE if the message should be propagated further
 *
 * This callback can be used to add custom message handlers. A new handler
 * is prepended to the list of handlers so that the newly registered one
 * gets to handle messages first.
 */
typedef gboolean (*MessageHandler)(Message *msg, gpointer user_data);

/** Do not propagate messages further */
#define LOGMSG_NO_PROPAGATE   ((gpointer)FALSE)
/** Do propagate messages further */
#define LOGMSG_PROPAGATE      ((gpointer)TRUE)

/** @brief syslog message handler
 *
 * The messages are propagated to syslog (using the standard syslog API)
 *
 * @note user_data should be LOGMSG_NO_PROPAGATE or LOGMSG_PROPAGATE.
 */
gboolean msg_syslog_handler(Message *msg, gpointer user_data);
/** @brief stderr message handler
*
* The messages are propagated to the standard error output
*
* @note user_data should be LOGMSG_NO_PROPAGATE or LOGMSG_PROPAGATE when adding handler.
*/
gboolean msg_stderr_handler(Message *msg, gpointer user_data);
/** @brief syslog message handler
*
* The messages are propagated to the standard error output using colours
*
* @note user_data should be LOGMSG_NO_PROPAGATE or LOGMSG_PROPAGATE when adding handler.
*/
gboolean msg_stderr_fancy_handler(Message *msg, gpointer user_data);
/** @brief syslog message handler
*
* The messages are propagated to syslog (using the standard syslog API)
*
* @note user_data should be LOGMSG_NO_PROPAGATE or LOGMSG_PROPAGATE when adding handler.
*/
gboolean msg_fail_handler(Message *msg, gpointer user_data);
/** @brief file message handler
 *
 * The messages are written to a file
 *
 * @note user_data should be a 'FILE *' pointer of the file the handler should write to.
 * @note propagates every message
 */
gboolean msg_file_handler(Message *msg, gpointer user_data);

/** @brief Emit message
 * @param msg Message that should be emitted
 * @param free_msg if TRUE, log_message will be destroyed after dispatch
 *
 * This function emits a message allready initialized
 *
 * @note Don't use this. If you cannot use the macros, use eighter log_format or log_vformat
 */
void log_message(Message *msg, gboolean free_msg);
/** @brief Emit formatted message
 * @param priority Priority of the message
 * @param msg Main string of the message
 * @param tag0 First tag associated to message (if NULL, no tags are present)
 * @param ... Further tags. The last one should be NULL denoting the end of the tag list.
 *
 * Emits a formatted message.
 *
 */
void log_format(gint priority, const gchar *msg, MessageTag *tag0, ...);
/** @brief va_list version of log_format
 * @see log_format
 */
void log_vformat(gint priority, const gchar *msg, MessageTag *tag0, va_list vl);

/** @brief Initialize log subsystem.
 *
 * Initialize the logging subsystem.
 *
 * @note The current implementation is empty but may not stay that way.
 */
void log_init();
/** @brief Clear log subsystem.
 *
 * Clears the logging subsystem. After this, it may be reinitialized with log_init
 */
void log_clear();

/** @brief Divert all messages
 * @param handler Message handler
 * @param user_data User data passed to handler
 *
 * Messages will be send to given handler. Cannot be reinitialized after this.
 * The divert mechanism is a way to pass all messages to a different messaging
 * system (e.g. an eventlogger, a different process, etc.)
 */
void log_divert(MessageHandler handler, gpointer user_data);

/** @brief Call message above given priority
 * @internal
 */
#define log_wrap(pri, msg...) \
  do { if (g_log_max_priority >= (pri)) { log_format((pri), msg); } } while (0)

/** @brief Emit an emergency message */
#define log_emerg(msg...)   log_wrap(LOG_EMERG, msg)
/** @brief Emit an alert message */
#define log_alert(msg...)   log_wrap(LOG_ALERT, msg)
/** @brief Emit a critical message */
#define log_crit(msg...)    log_wrap(LOG_CRIT, msg)
/** @brief Emit an error message */
#define log_error(msg...)   log_wrap(LOG_ERR, msg)
/** @brief Emit a warning message */
#define log_warn(msg...)    log_wrap(LOG_WARNING, msg)
/** @brief Emit a notice message */
#define log_notice(msg...)  log_wrap(LOG_NOTICE, msg)
/** @brief Emit an informative message */
#define log_info(msg...)    log_wrap(LOG_INFO, msg)
/** @brief Emit a debug message */
#define log_debug(msg...)   log_wrap(LOG_DEBUG, msg)

/** @brief The maximal priority dispatched
 * 
 * Below this priority level none of the messages will be emitted and
 * in the case of the macros, they will not even be formed.
 */
extern gint g_log_max_priority;

/** @brief Register a message handler
 * @param handler The message handler callback
 * @param max_priority Maximal priority value to be dispatched.
 * @param user_data User data to pass to handler (see built-in handlers)
 * @return Handle pointer for the message handler (see log_unregister_message_handler)
 *
 * This registers a message handler into the logging subsystem (globally).
 *
 * @note Since the most severe priority is LOG_EMERG with the value of zero,
 * the phrase "Maximal priority value" means the least severe value to be
 * dispatched. The lower this value is, the more messages are ignored.
 */
gpointer log_register_message_handler(MessageHandler handler,
  gint max_priority, gpointer user_data);
/** @brief Unregister message handler
 * @param handler Handle pointer as returned by log_register_message_handler)
 *
 * Unregister the message handler
 */
void log_unregister_message_handler(gpointer handler);

#endif
