#ifndef _TINU_UTILS_H
#define _TINU_UTILS_H

#include <glib/gtypes.h>
#include <glib/garray.h>

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

#endif
