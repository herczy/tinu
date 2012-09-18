#include <tinu.h>

TEST_FUNCTION(success, empty)
{
}

TEST_FUNCTION(success, assert)
{
  TINU_ASSERT_TRUE(1);
  TINU_ASSERT_FALSE(0);
}

TEST_FUNCTION(fail, assert)
{
  TINU_ASSERT_TRUE(0);
}

TEST_FUNCTION(fail, assertFatal)
{
  TINU_ASSERT_TRUE(0);
  FATAL(TINU_ASSERT_TRUE(0));
}

TEST_FUNCTION(fail, assertMisc)
{
  TINU_ASSERT_TRUE(0);
  TINU_ASSERT_TRUE(1);
  TINU_ASSERT_TRUE(2);
  TINU_ASSERT_TRUE(1);
  TINU_ASSERT_TRUE(0);
  TINU_ASSERT_TRUE(1);
  TINU_ASSERT_TRUE(2);
  TINU_ASSERT_TRUE(1);
  TINU_ASSERT_TRUE(0);

  TINU_ASSERT_STREQ("hello", "hello");
  TINU_ASSERT_STREQ("hello", "hello2");
  TINU_ASSERT_INTEQ(1, 1);
  TINU_ASSERT_INTEQ(1, -1);
}

TEST_FUNCTION(fail, segfault)
{
  *(int *)NULL = 1;
}

TEST_FUNCTION(leak, malloc)
{
  malloc(1024);
}

TEST_SETUP(leak, setup)
{
  return malloc(1024);
}

TEST_FUNCTION(leak, setup)
{
  FATAL(TINU_ASSERT_TRUE(context));
}

TEST_SETUP(complex, hash)
{
  GHashTable *self = g_hash_table_new(g_str_hash, g_str_equal);

  g_hash_table_insert(self, "val1", (gpointer)1);
  g_hash_table_insert(self, "val2", (gpointer)2);
  g_hash_table_insert(self, "val3", (gpointer)3);
  g_hash_table_insert(self, "val4", (gpointer)4);

  /* Note how we can assert in the setup phase too (also in the cleanup phase) */
  TINU_ASSERT_TRUE(g_hash_table_size(self) == 4);

  return self;
}

TEST_CLEANUP(complex, hash)
{
  GHashTable *self = (GHashTable *)context;
  g_hash_table_destroy(self);
}

TEST_FUNCTION(complex, hash)
{
  GHashTable *self = (GHashTable *)context;

  TINU_ASSERT_TRUE(g_hash_table_lookup(self, "val1") == (gpointer)1);
  TINU_ASSERT_TRUE(g_hash_table_lookup(self, "val2") == (gpointer)2);
  TINU_ASSERT_TRUE(g_hash_table_lookup(self, "val3") == (gpointer)3);
  TINU_ASSERT_TRUE(g_hash_table_lookup(self, "val4") == (gpointer)4);
}
