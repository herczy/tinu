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
Message *msg_copy(Message *self);
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
