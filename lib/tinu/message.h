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

#ifndef _TINU_MESSAGE_H
#define _TINU_MESSAGE_H

#include <stdarg.h>
#include <syslog.h>

#include <glib/gtypes.h>
#include <glib/garray.h>
#include <glib/gerror.h>

typedef struct _MessageTag
{
  gchar        *m_tag;
  gchar        *m_value;
} MessageTag;

typedef struct _Message
{
  gint          m_priority;
  gchar        *m_message;
  gint          m_tag_count;
  MessageTag  **m_tags;
} Message;

Message *msg_create(gint priority, const gchar *msg, MessageTag *tag0, ...);
Message *msg_vcreate(gint priority, const gchar *msg, MessageTag *tag0, va_list vl);
Message *msg_copy(const Message *self);
void msg_destroy(Message *self);

void msg_append(Message *self, MessageTag *tag0, ...);
void msg_vappend(Message *self, MessageTag *tag0, va_list vl);

MessageTag *msg_find_tag(Message *self, const gchar *name);

MessageTag *msg_tag_str(const gchar *tag, const gchar *string);
MessageTag *msg_tag_int(const gchar *tag, gint value);
MessageTag *msg_tag_hex(const gchar *tag, guint value);
MessageTag *msg_tag_ptr(const gchar *tag, const void *ptr);
MessageTag *msg_tag_bool(const gchar *tag, gboolean value);
MessageTag *msg_tag_errno();
MessageTag *msg_tag_dump(const gchar *tag, const gpointer data, gsize len);
MessageTag *msg_tag_gerror(GError *error);

MessageTag *msg_tag_vprintf(const gchar *tag, const gchar *fmt, va_list vl);
MessageTag *msg_tag_printf(const gchar *tag, const gchar *fmt, ...);

const gchar *msg_format_priority(gint priority);
gint msg_get_priority_value(const gchar *name);

gchar *msg_format_simple(const Message *self);

#endif
