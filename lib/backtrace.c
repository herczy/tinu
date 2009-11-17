#include <stdlib.h>
#include <string.h>
#include <execinfo.h>

#include <glib/gstring.h>

#include <applog.h>

#include <tinu/utils.h>
#include <tinu/backtrace.h>

#define MAX_BUF_SIZE      4096

struct _Backtrace
{
  gint        m_refcnt;

  guint32                 m_length;
  gchar                 **m_names;
};

struct _DumpLogUserData
{
  gint                     m_priority;
  const gchar             *m_prefix;
};

static void
_backtrace_dump_log_callback(const BacktraceEntry *entry, gpointer user_data)
{
  struct _DumpLogUserData *self = (struct _DumpLogUserData *)user_data;

  if (entry == BACKTRACE_ENTRY_INVALID)
    {
      log_format(self->m_priority, self->m_prefix,
                msg_tag_str("error", "invalid frame"), NULL);
      return;
    }

  log_format(self->m_priority, self->m_prefix,
            msg_tag_str("function", entry->m_function),
            msg_tag_str("file", entry->m_file),
            msg_tag_hex("offset", entry->m_offset),
            msg_tag_ptr("ptr", entry->m_ptr), NULL);
}

struct _DumpFileUserData
{
  FILE                 *m_file;
  gint                  m_indent;
};

static void
_backtrace_dump_file_callback(const BacktraceEntry *entry, gpointer user_data)
{
  struct _DumpFileUserData *self = (struct _DumpFileUserData *)user_data;

  if (entry == BACKTRACE_ENTRY_INVALID)
    {
      fprintf(self->m_file, "%*c<invalid frame>\n", self->m_indent, ' ');
      return;
    }

  if (entry->m_function)
    fprintf(self->m_file, "%*c%s\n", self->m_indent, ' ', entry->m_function);
  else
    fprintf(self->m_file, "%*c<file: %s>\n", self->m_indent, ' ', entry->m_file);
}

Backtrace *
backtrace_create(guint32 skip)
{
  return backtrace_create_depth(MAX_DEPTH, skip);
}

Backtrace *
backtrace_create_depth(guint32 depth, guint32 skip)
{
  gpointer buffer[MAX_DEPTH + 1];
  gchar **list;
  guint32 nptr, i;
  Backtrace *res;

  if (depth > MAX_DEPTH)
    {
      log_warn("Requested depth exceeded maximum depth",
                msg_tag_int("depth", depth),
                msg_tag_int("skip", skip), NULL);
    }

  if ((nptr = backtrace(buffer, MAX_DEPTH)) == -1)
    {
      log_error("Could not create backtrace object",
                msg_tag_int("depth", depth),
                msg_tag_int("skip", skip), NULL);
      return NULL;
    }

  if (nptr <= skip)
    {
      log_warn("Backtrace empty",
                msg_tag_int("depth", depth),
                msg_tag_int("skip", skip), NULL);

      res = t_new(Backtrace, 1);

      res->m_refcnt = 1;
      res->m_length = 0;
      res->m_names = NULL;
      return res;
    }

  list = backtrace_symbols(buffer + skip, nptr - skip);
  if (!list)
    {
      log_error("Could not get symbol names",
                msg_tag_int("depth", depth),
                msg_tag_int("skip", skip), NULL);
    }

  res = t_new(Backtrace, 1);
  res->m_refcnt = 1;
  res->m_length = nptr - skip;
  res->m_names = t_new(gchar *, res->m_length);

  for (i = 0; i < nptr - skip; i++)
    {
      res->m_names[i] = strdup(list[i]);
    }

  t_free(list);
  return res;
}

Backtrace *
backtrace_reference(Backtrace *self)
{
  self->m_refcnt++;
  return self;
}

void
backtrace_unreference(Backtrace *self)
{
  guint32 i;

  if (!self)
    return;

  self->m_refcnt--;

  if (self->m_refcnt == 0)
    {
      for (i = 0; i < self->m_length; i++)
        {
          t_free(self->m_names[i]);
        }
      t_free(self->m_names);
      t_free(self);
    }
}

guint32
backtrace_depth(const Backtrace *self)
{
  return self->m_length;
}

const gchar *
backtrace_line(const Backtrace *self, guint32 line)
{
  return self->m_names[line];
}

void
backtrace_dump_log(const Backtrace *self, const gchar *msg_prefix, gint priority)
{
  struct _DumpLogUserData ud = { priority, msg_prefix };
  backtrace_dump(self, &_backtrace_dump_log_callback, &ud);
}

void
backtrace_dump_file(const Backtrace *self, FILE *output, guint8 indent)
{
  struct _DumpFileUserData ud = { output, indent };
  backtrace_dump(self, &_backtrace_dump_file_callback, &ud);
}

void
backtrace_dump(const Backtrace *self, DumpCallback callback, void *user_data)
{
  BacktraceEntry entry;
  guint32 i;

  for (i = 0; i < self->m_length; i++)
    {
      if (!backtrace_entry_parse(&entry, self->m_names[i]))
        callback(BACKTRACE_ENTRY_INVALID, user_data);
      else
        callback(&entry, user_data);

      backtrace_entry_destroy(&entry);
    }
}

gint
backtrace_entry_parse(BacktraceEntry *self, const gchar *line)
{
  gchar *endp;
  gchar *pos;
  gint len = strlen(line);
  gint i = 0;

  gint start = 0, end = 0, plus = 0;

  gchar *tmp;

  memset(self, 0, sizeof(BacktraceEntry));

  /* Find offset (at the end) */
  for (i = len - 1; i >= 0 && (!start || !end); i--)
    {
      if (line[i] == ']')
        end = i;
      else if (line[i] == '[')
        start = i;
    }

  if (i >= 0)
    {
      self->m_ptr = (gpointer)strtoul(line + start + 3, &endp, 16);

      if (*endp != ']')
        {
          log_warn("Can not convert pointer in line",
                   msg_tag_str("line", line), NULL);
          self->m_ptr = NULL;
        }
      len = start;
    }

  for (start = end = 0, i = len - 1; i >= 0 && (!start || !end); i--)
    {
      if (line[i] == ')')
        end = i;
      else if (line[i] == '(')
        start = i;
      else if (line[i] == '+')
        plus = i;
    }

  if (i < 0)
    {
      for (; len >= 0 && isspace(line[len]); len--)
        ;

      if (len < 0)
        {
          log_error("Could not convert line at all",
                    msg_tag_str("line", line), NULL);
          return 0;
        }
      self->m_file = strndup(line, len - 1);
      return 1;
    }

  if (plus)
    {
      if (line[plus + 3] == '0' && line[plus + 4] == ')')
        {
          self->m_offset = 0;
        }
      else
        {
          self->m_offset = strtoul(line + plus + 3, &endp, 16);

          if (*endp != ')')
            {
              log_warn("Can not convert offset in line",
                       msg_tag_str("line", line), NULL);
              self->m_offset = 0;
            }
        }
    }
  else
    plus = end;

  self->m_file = strndup(line, start);
  self->m_function = strndup(line + start + 1, plus - start - 1);
  return 1;
}

void
backtrace_entry_destroy(BacktraceEntry *self)
{
  t_free(self->m_function);
  t_free(self->m_file);
}

MessageTag *
msg_tag_trace(const gchar *tag, const Backtrace *trace)
{
  MessageTag *res = t_new(MessageTag, 1);
  GString *str = g_string_new("");
  BacktraceEntry entry;
  guint32 i;

  for (i = 0; i < trace->m_length; i++)
    {
      if (!backtrace_entry_parse(&entry, trace->m_names[i]) || !entry.m_function)
        {
          g_string_append(str, "???, ");
        }
      else
        {
          g_string_append(str, entry.m_function);
          g_string_append(str, ", ");
        }
    }

  res->m_tag = strdup(tag);
  res->m_value = strndup(str->str, str->len - 2);

  g_string_free(str, TRUE);

  return res;
}

MessageTag *
msg_tag_trace_current(const gchar *tag, int skip)
{
  Backtrace *trace = backtrace_create(skip);
  MessageTag *res = msg_tag_trace(tag, trace);
  backtrace_unreference(trace);

  return res;
}
