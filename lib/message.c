#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>

#include <errno.h>

#include <glib/gtestutils.h>
#include <glib/gmem.h>
#include <glib/gstrfuncs.h>
#include <glib/gstring.h>

#include <tinu/message.h>
#include <tinu/log.h>

static const gchar *g_priority_names[] = {
  "emergency",
  "alert",
  "critical",
  "error",
  "warning",
  "notice",
  "info",
  "debug",
  NULL
};

static void
_msg_append_tag(Message *msg, MessageTag *tag, gint *size)
{
  if (msg->m_tag_count <= *size)
    {
      if (!*size)
        *size = 4;
      else
        *size <<= 1;

      msg->m_tags = g_renew(MessageTag *, msg->m_tags, *size);
    }
  msg->m_tags[msg->m_tag_count++] = tag;
}

static MessageTag *
_msg_generate_vtag(const gchar *tag, const gchar *fmt, va_list vl)
{
  MessageTag *res = g_new0(MessageTag, 1);

  res->m_tag = g_strdup(tag);
  res->m_value = g_strdup_vprintf(fmt, vl);

  return res;
}

static MessageTag *
_msg_generate_tag(const gchar *tag, const gchar *fmt, ...)
{
  va_list vl;
  MessageTag *res = g_new0(MessageTag, 1);

  va_start(vl, fmt);
  res->m_tag = g_strdup(tag);
  res->m_value = g_strdup_vprintf(fmt, vl);
  va_end(vl);

  return res;
}

static void
_msg_tag_clear(MessageTag *self)
{
  if (self)
    {
      g_free(self->m_tag);
      g_free(self->m_value);
      g_free(self);
    }
}

Message *
msg_create(gint priority, const gchar *msg, MessageTag *tag0, ...)
{
  va_list vl;
  Message *res;

  va_start(vl, tag0);
  res = msg_vcreate(priority, msg, tag0, vl);
  va_end(vl);

  return res;
}

Message *
msg_vcreate(gint priority, const gchar *msg, MessageTag *tag0, va_list vl)
{
  Message *self;
  MessageTag *tag;
  gint size = 0;

  self = g_new0(Message, 1);
  self->m_priority = priority;
  self->m_message = g_strdup(msg);

  for (tag = tag0; tag; tag = va_arg(vl, MessageTag *))
    _msg_append_tag(self, tag, &size);

  self->m_tags = g_renew(MessageTag *, self->m_tags, self->m_tag_count);
  return self;
}

Message *
msg_copy(Message *self)
{
  Message *res = g_new0(Message, 1);
  MessageTag *tag;
  gint i;

  res->m_priority = self->m_priority;
  res->m_message = g_strdup(self->m_message);
  res->m_tag_count = self->m_tag_count;

  for (i = 0; i < res->m_tag_count; i++)
    {
      tag = g_new0(MessageTag, 1);
      tag->m_tag = g_strdup(res->m_tags[i]->m_tag);
      tag->m_value = g_strdup(res->m_tags[i]->m_value);
      res->m_tags[i] = tag;
    }

  return res;
}

void
msg_destroy(Message *self)
{
  gint i;

  for (i = 0; i < self->m_tag_count; i++)
    {
      _msg_tag_clear(self->m_tags[i]);
    }
  g_free(self->m_tags);
  g_free(self->m_message);
  g_free(self);
}

void
msg_vappend(Message *self, MessageTag *tag0, va_list vl)
{
  MessageTag *tag;
  gint size = self->m_tag_count;

  for (tag = tag0; tag; tag = va_arg(vl, MessageTag *))
    _msg_append_tag(self, tag, &size);

  self->m_tags = g_renew(MessageTag *, self->m_tags, self->m_tag_count);
}

void
msg_append(Message *self, MessageTag *tag0, ...)
{
  va_list vl;

  va_start(vl, tag0);
  msg_vappend(self, tag0, vl);
  va_end(vl);
}

MessageTag *
msg_find_tag(Message *self, const gchar *name)
{
  gint i;

  for (i = 0; i < self->m_tag_count; i++)
    {
      if (!strcmp(self->m_tags[i]->m_tag, name))
        return self->m_tags[i];
    }
  return NULL;
}

MessageTag *
msg_tag_str(const gchar *tag, const gchar *string)
{
  return _msg_generate_tag(tag, "\"%s\"", string);
}

MessageTag *
msg_tag_int(const gchar *tag, gint value)
{
  return _msg_generate_tag(tag, "%d", value);
}

MessageTag *msg_tag_hex(const gchar *tag, gint value)
{
  return _msg_generate_tag(tag, "0x%x", value);
}

MessageTag *
msg_tag_ptr(const gchar *tag, const void *ptr)
{
  return _msg_generate_tag(tag, "%p", ptr);
}

MessageTag *
msg_tag_bool(const gchar *tag, gboolean value)
{
  return _msg_generate_tag(tag, "%s", value ? "true" : "false");
}

MessageTag *
msg_tag_errno()
{
  return msg_tag_printf("errno", "%d(\"%s\")", errno, strerror(errno));
}

MessageTag *
msg_tag_dump(const gchar *tag, const gpointer data, gsize len)
{
  MessageTag *res;
  GString *str = g_string_sized_new(len * 3);
  gsize i;
  const guint8 *ptr = (const guint8 *)data;

  for (i = 0; i < len; i++)
    {
      g_string_append_printf(str, "%02X ", ptr[i]);
    }

  res = g_new0(MessageTag, 1);
  res->m_tag = g_strdup(tag);
  res->m_value = g_string_free(str, FALSE);

  return res;
}

MessageTag *
msg_tag_gerror(GError *error)
{
  return _msg_generate_tag("g_error", "domain=%s code=%d msg=%s",
                           g_quark_to_string(error->domain),
                           error->code, error->message);
}

MessageTag *
msg_tag_vprintf(const gchar *tag, const gchar *fmt, va_list vl)
{
  return _msg_generate_vtag(tag, fmt, vl);
}

MessageTag *
msg_tag_printf(const gchar *tag, const gchar *fmt, ...)
{
  va_list vl;
  MessageTag *res;

  va_start(vl, fmt);
  res = msg_tag_vprintf(tag, fmt, vl);
  va_end(vl);

  return res;
}

const gchar *
msg_format_priority(gint priority)
{
  g_assert(priority >= LOG_EMERG && priority <= LOG_DEBUG);
  return g_priority_names[priority];
}

gint
msg_get_priority_value(const gchar *name)
{
  gint i;

  for (i = 0; g_priority_names[i]; i++)
    {
      if (!strcasecmp(g_priority_names[i], name))
        return i;
    }

  return -1;
}

gchar *
msg_format_simple(const Message *self)
{
  gint i;
  GString *str = g_string_sized_new(4096);

  g_string_append(str, self->m_message);

  for (i = 0; i < self->m_tag_count; i++)
    {
      g_string_append_printf(str, " [%s=%s]",
                             self->m_tags[i]->m_tag,
                             self->m_tags[i]->m_value);
    }

  return g_string_free(str, FALSE);
}
