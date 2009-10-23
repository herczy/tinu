#ifndef _TINU_UTILS_H
#define _TINU_UTILS_H

#include <glib/gtypes.h>
#include <glib/garray.h>

#include <applog.h>

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

#define t_assert(cond)                                                  \
  if (!(cond))                                                          \
    {                                                                   \
      log_crit("TINU internal assertion failed",                        \
               msg_tag_str("condition", # cond),                        \
               msg_tag_str("function", __PRETTY_FUNCTION__),            \
               msg_tag_str("filename", __FILE__),                       \
               msg_tag_int("line", __LINE__),                           \
               msg_tag_trace_current("trace", 3), NULL);                \
      exit(1);                                                          \
    }

#define t_raw_assert(cond)                                              \
  if (!(cond))                                                          \
    {                                                                   \
      fprintf(stderr, "Raw assert `" # cond "` failed (%s:%d (%s))\n",  \
              __FILE__, __LINE__, __PRETTY_FUNCTION__);                 \
      exit(1);                                                          \
    }

#endif
