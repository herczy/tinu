#define _GNU_SOURCE
#include <tinu.h>

void
test_success_2(gpointer context G_GNUC_UNUSED)
{
}

void
test_success_3(gpointer context G_GNUC_UNUSED)
{
  TINU_ASSERT_TRUE(1);
  TINU_ASSERT_FALSE(0);
}

void
test_fail_1(gpointer context G_GNUC_UNUSED)
{
  TINU_ASSERT_TRUE(0);
}

void
test_segv_1(gpointer context G_GNUC_UNUSED)
{
  *(int *)NULL = 1;
}

void
test_leak_1(gpointer context G_GNUC_UNUSED)
{
  malloc(1024);
}

gboolean
test_leak_2_setup(gpointer *context)
{
  *context = malloc(1024);
}

void
test_leak_2(gpointer context)
{
  TINU_ASSERT_TRUE(context);
}

gboolean
test_hash_1_setup(gpointer *context)
{
  GHashTable *self = g_hash_table_new(g_str_hash, g_str_equal);

  g_hash_table_insert(self, "val1", (gpointer)1);
  g_hash_table_insert(self, "val2", (gpointer)2);
  g_hash_table_insert(self, "val3", (gpointer)3);
  g_hash_table_insert(self, "val4", (gpointer)4);

  /* Note how we can assert in the setup phase too (also in the cleanup phase) */
  TINU_ASSERT_TRUE(g_hash_table_size(self) == 4);

  *context = self;
  return TRUE;
}

void
test_hash_1_cleanup(gpointer context)
{
  GHashTable *self = (GHashTable *)context;
  g_hash_table_destroy(self);
}

void
test_hash_1(gpointer context)
{
  GHashTable *self = (GHashTable *)context;

  TINU_ASSERT_TRUE(g_hash_table_lookup(self, "val1") == (gpointer)1);
  TINU_ASSERT_TRUE(g_hash_table_lookup(self, "val2") == (gpointer)2);
  TINU_ASSERT_TRUE(g_hash_table_lookup(self, "val3") == (gpointer)3);
  TINU_ASSERT_TRUE(g_hash_table_lookup(self, "val4") == (gpointer)4);
}

int
main(int argc, char *argv[])
{
  /* If all three functions are NULL, we pass */
  tinu_test_add("success", "nofunc", NULL, NULL, NULL);

  /* Empty functions succeed also by default */
  tinu_test_add("success", "empty", NULL, NULL, test_success_2);

  /* If all assertions pass, the test passes */
  tinu_test_add("success", "assert", NULL, NULL, test_success_3);

  /* If one of the asserts fail, the test will fail (also, the suite) */
  tinu_test_add("fail", "assert", NULL, NULL, test_fail_1);

  /* If the test case segfaults, the test will be flagged (also, it fails) */
  tinu_test_add("fail", "segfault", NULL, NULL, test_segv_1);

  /* --leakdet will show a leak here */
  tinu_test_add("leak", "malloc", NULL, NULL, test_leak_1);

  /* ... also, here */
  tinu_test_add("leak", "setup", test_leak_2_setup, NULL, test_leak_2);

  /* A bit more complex test */
  tinu_test_add("complex", "hash", test_hash_1_setup, test_hash_1_cleanup, test_hash_1);

  return tinu_main(&argc, &argv);
}
