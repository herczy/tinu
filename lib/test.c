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

#include <stdlib.h>
#include <string.h>

#include <ucontext.h>
#include <signal.h>

#include <tinu/utils.h>
#include <tinu/test.h>
#include <tinu/backtrace.h>
#include <tinu/leakwatch.h>
#include <tinu/config.h>

#ifndef sighandler_t
typedef void (*sighandler_t)(int);
#endif

#ifdef COREDUMPER_ENABLED
#include <coredumper.h>
#endif

#include <glib/gtestutils.h>

static gint g_signal = 0;

static sighandler_t g_sigsegv_handler = NULL;
static sighandler_t g_sigabrt_handler = NULL;

static TestContext *g_test_context_current = NULL;
static TestCase *g_test_case_current = NULL;

ucontext_t g_test_ucontext;

typedef struct _LeakInfo
{
  gboolean        m_enabled;
  GHashTable     *m_leaks;
  gpointer        m_watch_handle;
} LeakInfo;

typedef struct _TestHookItem
{
  TestHookCb      m_callback;
  gpointer        m_user_data;
} TestHookItem;

static void
_test_run_hooks(TestHookID hook_id, ...)
{
  va_list vl;
  CListIterator *iter;
  TestHookEntry *entry;

  g_assert (hook_id < TEST_HOOK_MAX);
  for (iter = clist_iter_new(g_test_context_current->m_hooks[hook_id]); clist_iter_next(iter); )
    {
      entry = (TestHookEntry *)clist_iter_data(iter);

      if (!entry->m_hook)
        continue;

      va_start(vl, hook_id);
      entry->m_hook(hook_id, g_test_context_current, entry->m_user_data, vl);
      va_end(vl);
    }
  clist_iter_done(iter);
}

static gboolean
_test_validate_name(const gchar *name)
{
  gint i;

  for (i = 0; name[i]; i++)
    {
      if (name[i] == '.' || name[i] == '=')
        return FALSE;
    }
  return TRUE;
}

void
_signal_handler(int signo)
{
  Backtrace *trace;

#ifdef COREDUMPER_ENABLED
  WriteCoreDump(core_file_name(g_test_context_current->m_core_dir,
    g_test_case_current->m_suite->m_name, g_test_case_current->m_name));
#endif
  trace = backtrace_create(3);
  log_error("Signal received while running a test",
            msg_tag_int("signal", signo),
            msg_tag_str("suite", g_test_case_current->m_suite->m_name),
            msg_tag_str("test", g_test_case_current->m_name), NULL);
  backtrace_dump_log(trace, "    ", LOG_ERR);
  backtrace_unreference(trace);

  g_signal = signo;

  if (g_signal == SIGABRT)
    _test_run_hooks(TEST_HOOK_SIGNAL_ABORT);
  else
    _test_run_hooks(TEST_HOOK_SIGNAL_SEGFAULT);

  setcontext(g_test_ucontext.uc_link);
}

void
_signal_on()
{
  g_signal = 0;

  g_sigsegv_handler = signal(SIGSEGV, _signal_handler);
  g_sigabrt_handler = signal(SIGABRT, _signal_handler);
}

void
_signal_off()
{
  signal(SIGSEGV, g_sigsegv_handler);
  signal(SIGABRT, g_sigabrt_handler);
}

void
_test_case_run_intern(TestContext *self, TestCase *test, TestCaseResult *result)
{
  gpointer ctx = NULL;

  g_test_case_current = test;
  _test_run_hooks(TEST_HOOK_BEFORE_TEST, test);

  if (test->m_setup)
    ctx = test->m_setup();

  if (test->m_test)
    test->m_test(ctx);

  if (test->m_cleanup)
    test->m_cleanup(ctx);

  *result = TEST_PASSED;
}

TestCaseResult
_test_case_run_single_test(TestContext *self, TestCase *test)
{
  TestCaseResult res = TEST_PASSED;

  GHashTable *leak_table = NULL;
  gpointer leak_handler = (self->m_leakwatch ? tinu_leakwatch_simple(&leak_table) : NULL);

  gpointer stack = g_malloc0(TEST_CTX_STACK_SIZE);

  ucontext_t main_ctx;

  if (self->m_sighandle)
    {
      _signal_on();

      if (getcontext(&g_test_ucontext) == -1)
        {
          log_error("Cannot get main context", msg_tag_errno(), NULL);
          goto internal_error;
        }

      g_test_ucontext.uc_stack.ss_sp = stack;
      g_test_ucontext.uc_stack.ss_size = TEST_CTX_STACK_SIZE;
      g_test_ucontext.uc_link = &main_ctx;
      makecontext(&g_test_ucontext, (void (*)())(&_test_case_run_intern), 3, self, test, &res);
    
      if (swapcontext(&main_ctx, &g_test_ucontext) == -1)
        {
          log_error("Cannot change context", msg_tag_errno(), NULL);
          goto internal_error;
        }
    
      g_free(stack);
    
      _signal_off();
    }
  else
    {
      g_signal = 0;
      _test_case_run_intern(self, test, &res);
    }

  if (g_signal == 0)
    {
      if (res == TEST_PASSED)
        {
          log_notice("Test case run successfull",
                     msg_tag_str("case", test->m_name),
                     msg_tag_str("suite", test->m_suite->m_name), NULL);
        }
      else
        {
          log_warn("Test case run failed",
                   msg_tag_str("case", test->m_name),
                   msg_tag_str("suite", test->m_suite->m_name), NULL);
        }
    }
  else
    {
      res = (g_signal == SIGSEGV ? TEST_SEGFAULT : TEST_FAILED);

      log_format(g_signal == SIGABRT ? LOG_WARNING : LOG_ERR, "Test case run returned with signal",
                 msg_tag_int("signal", g_signal),
                 msg_tag_str("case", test->m_name),
                 msg_tag_str("suite", test->m_suite->m_name), NULL);

    }

    if (leak_handler)
      {
        tinu_unregister_watch(leak_handler);
        if (res == TEST_PASSED)
          tinu_leakwatch_simple_dump(leak_table, LOG_WARNING);
        g_hash_table_destroy(leak_table);
      }

  _test_run_hooks(TEST_HOOK_AFTER_TEST, test, res);
  return res;

internal_error:
  return TEST_INTERNAL;
}

gboolean
_test_suite_run(TestContext *self, TestSuite *suite)
{
  gint i;
  gboolean res = TRUE;

  g_test_context_current = self;
  _test_run_hooks(TEST_HOOK_BEFORE_SUITE, suite);

  for (i = 0; i < suite->m_tests->len; i++)
    res &= TEST_PASSED ==
      _test_case_run_single_test(self, (TestCase *)g_ptr_array_index(suite->m_tests, i));

  log_format(res ? LOG_DEBUG : LOG_WARNING, "Test suite run complete",
            msg_tag_str("suite", suite->m_name),
            msg_tag_bool("result", res), NULL);

  _test_run_hooks(TEST_HOOK_AFTER_SUITE, suite, res);
  return res;
}

static TestSuite *
_test_suite_lookup(TestContext *self, const gchar *suite, gboolean new)
{
  guint i;
  TestSuite *res = NULL;

  for (i = 0; i < self->m_suites->len; i++)
    {
      res = (TestSuite *)g_ptr_array_index(self->m_suites, i);
      if (!strcmp(res->m_name, suite))
        goto exit;
    }

  if (!new)
    goto exit;

  res = t_new(TestSuite, 1);
  res->m_name = suite;
  res->m_tests = g_ptr_array_new();
  g_ptr_array_add(self->m_suites, res);

exit:
  return res;
}

static TestCase *
_test_lookup_case(TestSuite *suite, const gchar *test)
{
  TestCase *res;
  int i;

  for (i = 0; i < suite->m_tests->len; i++)
    {
      res = (TestCase *)g_ptr_array_index(suite->m_tests, i);
      if (!strcmp(res->m_name, test))
        return res;
    }

  return NULL;
}

void
test_context_init(TestContext *self)
{
  self->m_suites = g_ptr_array_new();
  memset(self->m_hooks, 0, sizeof(self->m_hooks));
}

void
test_context_destroy(TestContext *self)
{
  g_ptr_array_free(self->m_suites, TRUE);
  test_unregister_hook(self, TEST_HOOK_ALL, NULL, NULL);
}

void
test_add(TestContext *self,
         const gchar *suite_name,
         const gchar *test_name,
         TestSetup setup,
         TestCleanup cleanup,
         TestFunction func)
{
  TestSuite *suite;
  TestCase *res;

  if (!_test_validate_name(suite_name))
    {
      log_crit("Invalid suite name (names cannot contain '.' and '=' characters)",
               msg_tag_str("name", suite_name), NULL);
      g_assert(0);
    }

  if (!_test_validate_name(test_name))
    {
      log_crit("Invalid test case name (names cannot contain '.' and '=' characters)",
               msg_tag_str("name", test_name), NULL);
      g_assert(0);
    }

  suite = _test_suite_lookup(self, suite_name, TRUE);
  res = _test_lookup_case(suite, test_name);

  if (res)
    {
      log_warn("Test case already exists",
               msg_tag_str("suite", suite_name),
               msg_tag_str("test", test_name), NULL);

      res->m_setup = setup;
      res->m_cleanup = cleanup;
      res->m_test = func;
      return;
    }

  res = t_new(TestCase, 1);
  res->m_suite = suite;
  res->m_name = test_name;
  res->m_setup = setup;
  res->m_cleanup = cleanup;
  res->m_test = func;

  g_ptr_array_add(suite->m_tests, res);

  log_debug("Test case added",
            msg_tag_str("suite", suite_name),
            msg_tag_str("case", test_name), NULL);
}

void
test_register_hook(TestContext *self, TestHookID hook_id, TestHookCb hook, gpointer user_data)
{
  gint i;
  TestHookEntry *entry;

  if (hook_id != TEST_HOOK_ALL)
    {
      g_assert (hook_id < TEST_HOOK_MAX);

      entry = g_new0(TestHookEntry, 1);
      entry->m_hook = hook;
      entry->m_user_data = user_data;
      self->m_hooks[hook_id] = clist_append(self->m_hooks[hook_id], entry);
    }
  else
    {
      for (i = 0; i < TEST_HOOK_MAX; i++)
        test_register_hook(self, i, hook, user_data);
    }
}

void
test_register_multiple_hooks(TestContext *self, TestHookCb *hooks, gpointer user_data)
{
  gint id;

  for (id = 0; id < TEST_HOOK_MAX; id++)
    {
      if (hooks[id])
        test_register_hook(self, id, hooks[id], user_data);
    }
}

void
test_unregister_hook(TestContext *self, TestHookID hook_id, TestHookCb hook, gpointer user_data)
{
  CListIterator *iter;
  TestHookEntry *entry;
  gint i;

  if (hook_id == TEST_HOOK_ALL)
    {
      for (i = 0; i < TEST_HOOK_MAX; i++)
        test_unregister_hook(self, i, NULL, NULL);
      return;
    }

  g_assert (hook_id < TEST_HOOK_MAX);

  if (hook == NULL)
    {
      clist_destroy(self->m_hooks[hook_id], g_free);
      self->m_hooks[hook_id] = NULL;
      return;
    }

  for (iter = clist_iter_new(self->m_hooks[hook_id]); clist_iter_next(iter); )
    {
      entry = (TestHookEntry *)clist_iter_data(iter);
      if (hook == entry->m_hook && (!user_data || entry->m_user_data == user_data))
        clist_iter_remove(iter, g_free);
    }
  clist_iter_done(iter);
  self->m_hooks[hook_id] = NULL;
}

void
test_unregister_multiple_hooks(TestContext *self, TestHookCb *hooks, gpointer user_data)
{
  gint id;

  for (id = 0; id < TEST_HOOK_MAX; id++)
    {
      if (hooks[id])
        test_unregister_hook(self, id, hooks[id], user_data);
    }
}

gboolean
tinu_test_all_run(TestContext *self)
{
  gboolean res = TRUE, suite_res;
  gint i;

  for (i = 0; i < self->m_suites->len; i++)
    {
      suite_res = _test_suite_run(self, (TestSuite *)g_ptr_array_index(self->m_suites, i));
      res &= suite_res;
    }

  return res;
}

gboolean
tinu_test_suite_run(TestContext *self, const gchar *suite_name)
{
  TestSuite *suite = _test_suite_lookup(self, suite_name, FALSE);

  if (!suite)
    {
      log_error("Suite does not exist", msg_tag_str("suite", suite_name), NULL);
      return FALSE;
    }

  return _test_suite_run(self, suite);
}

void
tinu_test_assert(gboolean condition, const gchar *assert_type, const gchar *condstr,
  const gchar *file, const gchar *func, gint line, MessageTag *tag0, ...)
{
  va_list vl;
  Message *msg;

  _test_run_hooks(TEST_HOOK_ASSERT, condition, file, func, line);

  if ((condition && g_log_max_priority >= LOG_DEBUG) ||
      (!condition && g_log_max_priority >= LOG_ERR))
    {
      va_start(vl, tag0);
      if (condition)
        {
          msg = msg_create(LOG_DEBUG, "Assertion passed", NULL);
        }
      else
        {
          msg = msg_create(LOG_ERR, "Assertion failed", NULL);
        }

      msg_append(msg, msg_tag_str("condition", condstr),
                      msg_tag_str("type", assert_type),
                      msg_tag_str("file", file),
                      msg_tag_str("function", func),
                      msg_tag_int("line", line), NULL);
      msg_vappend(msg, tag0, vl);
      log_message(msg, TRUE);
    }

  if (!condition)
    abort();
}
