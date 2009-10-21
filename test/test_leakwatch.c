#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <tinu/utils.h>
#include <tinu/log.h>
#include <tinu/leakwatch.h>

void
__test_hook(LeakwatchOperation op, gpointer oldptr, gpointer ptr, gsize size, Backtrace *trace, gpointer user_data)
{
  log_notice("Memory operation",
             msg_tag_ptr("oldptr", oldptr), msg_tag_ptr("ptr", ptr), msg_tag_int("oldsize", size),
             msg_tag_trace("trace", trace), msg_tag_ptr("userdata", user_data), NULL);
}

void
dump_leaks(gpointer ptr, gpointer chunk, gpointer user_data G_GNUC_UNUSED)
{
  MemoryEntry *self = (MemoryEntry *)chunk;

  log_notice("Memory leak found", msg_tag_bool("leak", TRUE), msg_tag_ptr("pointer", ptr), msg_tag_int("size", self->m_size),
    (self->m_origin ? msg_tag_trace("original", self->m_origin) : msg_tag_str("original", "<unknown>")),
    (self->m_last ? msg_tag_trace("last", self->m_last) : msg_tag_str("last", "<unknown>")), NULL);
}

gboolean
leakinfo(Message *msg, gpointer user_data)
{
  if (msg_find_tag(msg, (const gchar *)user_data))
    msg_stderr_handler(msg, NULL);
  return TRUE;
}

int
main()
{
  GHashTable *leaks = NULL;
  gchar *x;
  gpointer /* ptr1, ptr2, ptr3, */ watch;

  tinu_register_message_handler(leakinfo, LOG_DEBUG, (gpointer)"leak");
  tinu_register_message_handler(msg_fail_handler, LOG_CRIT, (gpointer)LOG_CRIT);

  /* tinu_leakwatch_init(); */

  watch = tinu_leakwatch_simple(&leaks);

  x = t_new(gchar, 4096);

  //ptr1 = tinu_register_watch(__test_hook, NULL);
  free(x);

  //ptr2 = tinu_register_watch(__test_hook, (gpointer)-1);
  x = malloc(4096);

  //ptr3 = tinu_register_watch(__test_hook, (gpointer)0xFFFF);

  free(realloc(realloc(malloc(1024), 512), 128));
  free(x);

  x = malloc(2 * 4096);
  //tinu_unregister_watch(ptr3);
  //tinu_unregister_watch(ptr2);

  //free(x);
  //tinu_unregister_watch(ptr1);
  tinu_unregister_watch(watch);

  g_hash_table_foreach(leaks, dump_leaks, NULL);
  g_hash_table_destroy(leaks);

  return 0;
}
