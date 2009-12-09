#include <stdlib.h>
#include <string.h>

#include <sys/mman.h>
#include <unistd.h>

#include <tinu/utils.h>

#include <glib/gmem.h>

struct _SharedMemory
{
  gsize       m_size;
  gsize       m_aligned;

  gchar       m_ptr[0];
};

static inline gsize
_align(gsize size)
{
  gsize pg = getpagesize();
  return (size / pg + 1) * pg;
}

gpointer 
__allocate(gsize size)
{
  gpointer ptr;

  t_raw_assert(size > 0);
  ptr = g_try_malloc0(size);
  t_raw_assert(ptr);
  return ptr;
}

gpointer 
__reallocate(gpointer ptr, gsize size)
{
  gpointer res;
  if (size == 0)
    {
      __free(ptr);
      return NULL;
    }

  res = g_try_realloc(ptr, size);
  t_raw_assert(res);
  return res;
}

void
__free(gpointer ptr)
{
  if (ptr)
    g_free(ptr);
}

gchar *
core_file_name(const gchar *dir, const gchar *suite, const gchar *test)
{
  static gchar res[1024];

  snprintf(res, sizeof(res), "%s/core.%s.%s", dir, suite, test);
  return res;
}

gchar *
strndup(const gchar *str, gsize n)
{
  gint len = strlen(str);
  gchar *res;

  if (len > n)
    len = n;

  res = t_new(gchar, len + 1);
  memcpy(res, str, len);

  return res;
}
