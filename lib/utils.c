#include <stdlib.h>
#include <string.h>

#include <sys/mman.h>
#include <unistd.h>

#include <tinu/log.h>
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

gboolean
t_read(const GByteArray *src, gpointer dest, gsize len, gsize *pos)
{
  MessageTag *tag;
  if (*pos + len > src->len)
    return FALSE;

  memcpy(dest, src->data + *pos, len);
  *pos += len;

  return TRUE;
}

gboolean
t_gets(const GByteArray *src, gchar **dest, gint *pos)
{
  gint len;
  if (!t_read(src, (gpointer)&len, sizeof(len), pos))
    return FALSE;

  *dest = t_new(gchar, len);
  if (!t_read(src, (gpointer)*dest, len, pos))
    {
      t_free(*dest);
      *dest = NULL;
      return FALSE;
    }
  return TRUE;
}

void
t_puts(GByteArray *dest, const gchar *src)
{
  gint len = strlen(src) + 1;
  g_byte_array_append(dest, (const gpointer)&len, sizeof(len));
  g_byte_array_append(dest, (const gpointer)src, len);
}

/*gboolean
t_write(gint fd, const gpointer src, gsize size)
{
  return write(fd, src, size) == size;
}

gboolean
t_read(gint fd, gpointer dst, gsize size)
{
  return read(fd, dst, size) == size;
}

gboolean
t_puts(gint fd, const gchar *str)
{
  int len = strlen(str) + 1;
  return t_putv(fd, len) && t_write(fd, (const gpointer)str, len);
}

gboolean
t_gets(gint fd, gchar **str)
{
  int len;
  
  if (!t_getv(fd, len))
    return FALSE;

  *str = t_new(gchar, len);
  if (!t_gets(fd, str))
    {
      t_free(*str);
      *str = NULL;
      return FALSE;
    }

  return TRUE;
}*/

#if 0
gpointer
shmalloc(gsize size)
{
  gsize alloc = _align(size + sizeof(struct _SharedMemory));
  struct _SharedMemory *res;
  
  res = (struct _SharedMemory *)mmap(NULL, alloc, PROT_READ | PROT_WRITE,
                                     MAP_SHARED | MAP_ANONYMOUS, -1, 0);

  if (!res)
    log_abort("Could not map requested size", NULL);

  res->m_size = size;
  res->m_allocated = alloc;
  return res + 1;
}

gpointer
shrealloc(gpointer ptr, gsize size)
{
  struct _SharedMemory *self = (struct _SharedMemory *)ptr - 1;
  struct _SharedMemory *res;
  gsize alloc;

  if (self->m_allocated < size)
    {
      self->m_size = size;
      return ptr;
    }

#ifdef _GNU_SOURCE
  alloc = _align(size + sizeof(struct _SharedMemory));

  res = (struct _SharedMemory *)mremap(self, self->m_allocated, alloc);
  if (!res)
    log_abort("Cannot remap memory", NULL);

  res->m_size = size;
  res->m_allocated = alloc;

  return res + 1;
#else
  res = (struct _SharedMemory *)shmalloc(size);
  memcpy(res, ptr, self->m_size);
  shfree(ptr);

  return res;
#endif
}

void
shfree(gpointer ptr)
{
  struct _SharedMemory *self = (struct _SharedMemory *)ptr - 1;
  munmap(self, self->m_allocated);
}
#endif

