/* TINU - Unittesting framework
*
* Copyright (c) 2009, Viktor Hercinger <hercinger.viktor@gmail.com>
* All rights reserved.
* 
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*     * Redistributions of source code must retain the above copyright
*       notice, this list of conditions and the following disclaimer.
*     * Redistributions in binary form must reproduce the above copyright
*       notice, this list of conditions and the following disclaimer in the
*       documentation and/or other materials provided with the distribution.
*     * Neither the name of the original author (Viktor Hercinger) nor the
*       names of its contributors may be used to endorse or promote products
*       derived from this software without specific prior written permission.
* 
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
* ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER BE LIABLE FOR ANY
* DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
* LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
* ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
* SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*
* Author(s): Viktor Hercinger <hercinger.viktor@gmail.com>
*/

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
test_fail_2(gpointer context G_GNUC_UNUSED)
{
  TINU_ASSERT_TRUE(0);
  FATAL(TINU_ASSERT_TRUE(0));
}

void
test_fail_3(gpointer context G_GNUC_UNUSED)
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

gpointer
test_leak_2_setup(void)
{
  return malloc(1024);
}

void
test_leak_2(gpointer context)
{
  FATAL(TINU_ASSERT_TRUE(context));
}

gpointer
test_hash_1_setup(void)
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

  /* If one of the critical asserts fail, the test will fail (also, the suite)
   * and the test execution will be aborted. */
  tinu_test_add("fail", "assert_fatal", NULL, NULL, test_fail_2);

  /* All asserts are run, even after the fail, since none of them are critical */
  tinu_test_add("fail", "assert_misc", NULL, NULL, test_fail_3);

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
