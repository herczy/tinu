#define _GNU_SOURCE
#include <stdlib.h>
#include <string.h>
#include <execinfo.h>

#include <dlfcn.h>

#include <glib/gstring.h>
#include <glib/gstrfuncs.h>
#include <glib/gmem.h>

#include <tinu/utils.h>
#include <tinu/backtrace.h>
#include <tinu/log.h>

#include <config.h>

#if HAVE_DWARF == 1
#include <tinu/dwarf.h>

static gboolean g_backtrace_init = FALSE;
static DwarfHandle *g_backtrace_dwarf;

extern const gchar *g_runtime_name;

static void
_backtrace_cleanup()
{
  dw_destroy(g_backtrace_dwarf);
}

static inline void
_backtrace_init()
{
  if (!g_backtrace_init)
    {
      g_backtrace_init = TRUE;
      g_backtrace_dwarf = dw_new(g_runtime_name);
    }
}

static inline gboolean
_backtrace_get_lineinfo(const BacktraceEntry *entry, const gchar **file, guint32 *lineno)
{
  const DwarfEntry *dw_entry;

  if (!g_backtrace_dwarf)
    return FALSE;

  dw_entry = dw_lookup(g_backtrace_dwarf, entry->m_ptr, 0x30);

  if (!dw_entry)
    return FALSE;

  *file = g_quark_to_string(dw_entry->m_source);
  *lineno = dw_entry->m_lineno;
  return TRUE;
}
#else
#define _backtrace_init()
#endif

#define MAX_BUF_SIZE      4096

struct _Backtrace
{
  gint                    m_refcnt;

  guint32                 m_length;
  gpointer               *m_symbols;
};

struct _DumpLogUserData
{
  gint                     m_priority;
  const gchar             *m_prefix;
};

static BacktraceEntry *
_backtrace_resolve_info(const gpointer addr)
{
  Dl_info info;
  BacktraceEntry *res = NULL;

  if (!addr)
    return NULL;

  if (dladdr(addr, &info) != 0);
    {
      res = g_new0(BacktraceEntry, 1);

      res->m_ptr = addr;
      res->m_offset = addr - info.dli_saddr;
      res->m_function = g_strdup(info.dli_sname);
      res->m_file = g_strdup(info.dli_fname);
    }
  return res;
}

static void
_backtrace_dump_log_callback(const BacktraceEntry *entry, gpointer user_data)
{
  struct _DumpLogUserData *self = (struct _DumpLogUserData *)user_data;
  const gchar *src = NULL;
  guint32 line = 0;

  if (entry == BACKTRACE_ENTRY_INVALID)
    {
      log_format(self->m_priority, self->m_prefix,
                msg_tag_str("error", "invalid frame"), NULL);
      return;
    }

  backtrace_resolv_lines(entry, &src, &line);

  log_format(self->m_priority, self->m_prefix,
            msg_tag_str("function", entry->m_function),
            msg_tag_str("file", entry->m_file),
            msg_tag_hex("offset", entry->m_offset),
            msg_tag_ptr("ptr", entry->m_ptr),
            msg_tag_printf("location", "%s:%d", src, line), NULL);
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

  const gchar *src = NULL;
  guint32 line = 0;

  if (entry == BACKTRACE_ENTRY_INVALID)
    {
      fprintf(self->m_file, "%*c<invalid frame>\n", self->m_indent, ' ');
      return;
    }

  if (entry->m_function)
    fprintf(self->m_file, "%*c%s\n", self->m_indent, ' ', entry->m_function);
  else
    fprintf(self->m_file, "%*c<file: %s>\n", self->m_indent, ' ', entry->m_file);

  if (backtrace_resolv_lines(entry, &src, &line))
    fprintf(self->m_file, "\tlocation: %s:%d\n", src, line);
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
  guint32 nptr, i;
  Backtrace *res;

  _backtrace_init();

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
      res->m_symbols = NULL;
      return res;
    }

  res = t_new(Backtrace, 1);
  res->m_refcnt = 1;
  res->m_length = nptr - skip;
  res->m_symbols = g_new0(gpointer, nptr - skip);
  memcpy(res->m_symbols, buffer + skip, (nptr - skip) * sizeof(gpointer));

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
      t_free(self->m_symbols);
      t_free(self);
    }
}

guint32
backtrace_depth(const Backtrace *self)
{
  return self->m_length;
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
  BacktraceEntry *entry;
  guint32 i;

  for (i = 0; i < self->m_length; i++)
    {
      if (NULL != (entry = _backtrace_resolve_info(self->m_symbols[i])))
        {
          callback(entry, user_data);
          backtrace_entry_destroy(entry);
          t_free(entry);
        }
      else
        callback(BACKTRACE_ENTRY_INVALID, user_data);

    }
}

gboolean
backtrace_resolv_lines(const BacktraceEntry *entry, const gchar **src, guint32 *line)
{
#if HAVE_DWARF == 1
  return _backtrace_get_lineinfo(entry, src, line);
#else
  return FALSE;
#endif
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
  BacktraceEntry *entry;
  guint32 i;

  for (i = 0; i < trace->m_length; i++)
    {
      if ((NULL != (entry = _backtrace_resolve_info(trace->m_symbols[i]))) && entry->m_function)
        {
          g_string_append(str, entry->m_function);
          g_string_append(str, ", ");

          backtrace_entry_destroy(entry);
          t_free(entry);
        }
      else
        g_string_append(str, "???, ");
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
