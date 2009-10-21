#ifndef _TINU_LEAKWATCH_H
#define _TINU_LEAKWATCH_H

#include <glib/ghash.h>

#include <tinu/backtrace.h>

typedef enum
{
  LEAKWATCH_OPERATION_MALLOC,
  LEAKWATCH_OPERATION_REALLOC,
  LEAKWATCH_OPERATION_FREE
} LeakwatchOperation;

typedef void (*AllocCallback)(LeakwatchOperation operation,
                              gpointer oldptr, gpointer ptr, gsize size,
                              Backtrace *trace,
                              gpointer user_data);

gpointer tinu_register_watch(AllocCallback callback, gpointer user_data);
gboolean tinu_unregister_watch(gpointer handle);

typedef struct _MemoryEntry
{
  gpointer      m_ptr;
  gsize         m_size;

  Backtrace    *m_origin;
  Backtrace    *m_last;
} MemoryEntry;

/* The hash table has gpointers as keys and 'struct _MemoryEntry's as values */
gpointer tinu_leakwatch_simple(GHashTable **result);
void tinu_leakwatch_simple_dump(GHashTable *result, gint loglevel);

#endif
