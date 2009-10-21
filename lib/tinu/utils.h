#ifndef _TINU_UTILS_H
#define _TINU_UTILS_H

#include <glib/gtypes.h>
#include <glib/garray.h>

// TODO: Should make an abort if malloc fails
#define t_new(type, count) \
  (type *)__allocate(sizeof(type) * (count))

#define t_newp(bytes) \
  __allocate(bytes)

#define t_realloc(ptr, type, count) \
  ((ptr) = (type *)__reallocate(ptr, sizeof(type) * (count)))

#define t_reallocp(ptr, bytes) \
  ((ptr) = __reallocate((ptr), (bytes)))

#define t_free(ptr) __free(ptr)

gpointer __allocate(gsize size);
gpointer __reallocate(gpointer ptr, gsize size);
void     __free(gpointer ptr);

#define _strdup(str) ((str) ? strdup(str) : NULL)
gchar *strndup(const gchar *str, gsize n);

/* gpointer shmalloc(gsize size);
gpointer shrealloc(gpointer ptr, gsize size);
void shfree(gpointer ptr); */

gboolean t_read(const GByteArray *src, gpointer dest, gsize len, gsize *pos);
gboolean t_gets(const GByteArray *src, gchar **dest, gint *pos);

void t_puts(GByteArray *dest, const gchar *src);

#define t_putv(fd, p)   g_byte_array_append((fd), (const gpointer)&(p), sizeof(p))
#define t_putp(fd, p)   g_byte_array_append((fd), (const gpointer)(p), sizeof(*(p)))
#define t_getv(fd, p)   t_read((fd), (gpointer)&(p), sizeof(p), &pos)
#define t_getp(fd, p)   t_read((fd), (gpointer)(p), sizeof(*(p)), &pos)

#endif
