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

static sighandler_t g_sigsegv_handler = NULL;
static sighandler_t g_sigabrt_handler = NULL;

static TestContext *g_test_context_current = NULL;
static TestCase *g_test_case_current = NULL;
static TestCaseResult g_test_case_current_result = TEST_NONE;

ucontext_t g_test_ucontext;

typedef struct _LeakInfo
{
  gboolean        m_enabled;
  GHashTable     *m_leaks;
  gpointer        m_watch_handle;
} LeakInfo;

typedef struct _TestSimplifiedFunctions
{
  TestSetupSimple         m_setup;
  TestCleanupSimple       m_cleanup;
  TestFunctionSimple      m_function;
} TestSimplifiedFunctions;

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

  if (signo == SIGABRT)
    {
      _test_run_hooks(TEST_HOOK_SIGNAL_ABORT);
      g_test_case_current_result = TEST_ABORT;
    }
  else
    {
      _test_run_hooks(TEST_HOOK_SIGNAL_SEGFAULT);
      g_test_case_current_result = TEST_SEGFAULT;
    }

  setcontext(g_test_ucontext.uc_link);
}

void
_signal_on()
{
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
_test_case_run_intern(TestContext *self, TestCase *test)
{
  gpointer ctx = NULL;

  g_test_case_current = test;
  _test_run_hooks(TEST_HOOK_BEFORE_TEST, test);

  if (test->m_setup)
    ctx = test->m_setup(test);

  if (test->m_test)
    test->m_test(test, ctx);

  if (test->m_cleanup)
    test->m_cleanup(test, ctx);

  if (g_test_case_current_result == TEST_NONE)
    g_test_case_current_result = TEST_PASSED;
}

TestCaseResult
_test_case_run_single_test(TestContext *self, TestCase *test)
{
  GHashTable *leak_table = NULL;
  gpointer leak_handler = (self->m_leakwatch ? tinu_leakwatch_simple(&leak_table) : NULL);

  gpointer stack = g_malloc0(TEST_CTX_STACK_SIZE);
  gsize leaked_bytes;

  ucontext_t main_ctx;

  g_test_case_current_result = TEST_NONE;

  if (self->m_sighandle)
    {
      _signal_on();

      if (getcontext(&g_test_ucontext) == -1)
        {
          log_error("Cannot get main context", msg_tag_errno(), NULL);
          g_test_case_current_result = TEST_INTERNAL;
          goto test_case_run_done;
        }

      g_test_ucontext.uc_stack.ss_sp = stack;
      g_test_ucontext.uc_stack.ss_size = TEST_CTX_STACK_SIZE;
      g_test_ucontext.uc_link = &main_ctx;
      makecontext(&g_test_ucontext, (void (*)())(&_test_case_run_intern), 2, self, test);
    
      if (swapcontext(&main_ctx, &g_test_ucontext) == -1)
        {
          log_error("Cannot change context", msg_tag_errno(), NULL);
          g_test_case_current_result = TEST_INTERNAL;
          goto test_case_run_done;
        }
    
      g_free(stack);
    
      _signal_off();
    }
  else
    _test_case_run_intern(self, test);

test_case_run_done:
  switch (g_test_case_current_result)
    {
      case TEST_PASSED :
        log_notice("Test case run successfull",
                   msg_tag_str("case", test->m_name),
                   msg_tag_str("suite", test->m_suite->m_name), NULL);
        break;

      case TEST_FAILED :
        log_warn("Test case run failed",
                 msg_tag_str("case", test->m_name),
                 msg_tag_str("suite", test->m_suite->m_name), NULL);
        break;

      case TEST_ABORT :
        log_error("Test case run failed abruptly",
                 msg_tag_str("case", test->m_name),
                 msg_tag_str("suite", test->m_suite->m_name), NULL);
        break;

      case TEST_SEGFAULT :
        log_error("Test case run produced a segmentation fault",
                 msg_tag_str("case", test->m_name),
                 msg_tag_str("suite", test->m_suite->m_name), NULL);
        break;

      case TEST_INTERNAL :
        log_crit("Test case run failed due to an internal error",
                 msg_tag_str("case", test->m_name),
                 msg_tag_str("suite", test->m_suite->m_name), NULL);
        break;

      default :
        g_assert_not_reached();
    }

  if (leak_handler)
    {
      tinu_unregister_watch(leak_handler);

      // Send summary to hooks
      leaked_bytes = tinu_leakwatch_summary(leak_table);
      _test_run_hooks(TEST_HOOK_LEAKINFO, test, leaked_bytes);

      // Dump statistics
      if (g_test_case_current_result == TEST_PASSED)
        tinu_leakwatch_simple_dump(leak_table, LOG_WARNING);

      g_hash_table_destroy(leak_table);
    }

  _test_run_hooks(TEST_HOOK_AFTER_TEST, test, g_test_case_current_result);
  return g_test_case_current_result;
}

gboolean
_test_suite_run(TestContext *self, TestSuite *suite, TestCase *test)
{
  gint i;
  gboolean res = TRUE;

  g_test_context_current = self;
  _test_run_hooks(TEST_HOOK_BEFORE_SUITE, suite);

  if (test)
    {
      res = TEST_PASSED ==
        _test_case_run_single_test(self, test);
    }
  else
    {
      for (i = 0; i < suite->m_tests->len; i++)
        res &= TEST_PASSED ==
          _test_case_run_single_test(self, (TestCase *)g_ptr_array_index(suite->m_tests, i));
    }

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

static gpointer
_test_setup_wrapper(TestCase *self)
{
  TestSimplifiedFunctions *func = (TestSimplifiedFunctions *)self->m_user_data;
  return func->m_setup ? func->m_setup() : NULL;
}

static void
_test_cleanup_wrapper(TestCase *self, gpointer context)
{
  TestSimplifiedFunctions *func = (TestSimplifiedFunctions *)self->m_user_data;
  if (func->m_cleanup)
    func->m_cleanup(context);
}

static void
_test_func_wrapper(TestCase *self, gpointer context)
{
  TestSimplifiedFunctions *func = (TestSimplifiedFunctions *)self->m_user_data;
  if (func->m_function)
    func->m_function(context);
}

const gchar *
test_result_name(TestCaseResult result)
{
  g_assert (result > TEST_NONE && result <= TEST_INTERNAL);
  return tinu_lookup_key(TestCaseResult_names, result, NULL);
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
  gint i, j;
  TestSuite *test_suite;
  TestCase *test_case;

  for (i = 0; i < self->m_suites->len; i++)
    {
      test_suite = (TestSuite *)self->m_suites->pdata[i];

      for (j = 0; j < test_suite->m_tests->len; j++)
        {
          test_case = (TestCase *)test_suite->m_tests->pdata[j];

          if (test_case->m_user_data_cleanup)
            test_case->m_user_data_cleanup(test_case->m_user_data);

          g_free(test_case);
        }

      g_ptr_array_free(test_suite->m_tests, TRUE);
      g_free(test_suite);
    }

  g_ptr_array_free(self->m_suites, TRUE);
  test_unregister_hook(self, TEST_HOOK_ALL, NULL, NULL);
}

void
test_add(TestContext *self,
         const gchar *suite_name,
         const gchar *test_name,
         TestSetupSimple setup,
         TestCleanupSimple cleanup,
         TestFunctionSimple func)
{
  TestSimplifiedFunctions *user_data = g_new0(TestSimplifiedFunctions, 1);
  user_data->m_setup = setup;
  user_data->m_cleanup = cleanup;
  user_data->m_function = func;

  test_add_extended(self,
                    suite_name,
                    test_name,
                    _test_setup_wrapper,
                    _test_cleanup_wrapper,
                    _test_func_wrapper,
                    user_data,
                    g_free);
}

void
test_add_extended(TestContext *self,
                  const gchar *suite_name,
                  const gchar *test_name,
                  TestSetup setup,
                  TestCleanup cleanup,
                  TestFunction func,
                  gpointer user_data,
                  CleanupFunction user_data_cleanup)
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
  res->m_user_data = user_data;
  res->m_user_data_cleanup = user_data_cleanup;

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
      suite_res = _test_suite_run(self, (TestSuite *)g_ptr_array_index(self->m_suites, i), NULL);
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

  return _test_suite_run(self, suite, NULL);
}

gboolean
tinu_test_case_run(TestContext *self, const gchar *suite_name, const gchar *test_name)
{
  TestSuite *suite = _test_suite_lookup(self, suite_name, FALSE);
  TestCase *test;

  if (!suite)
    {
      log_error("Suite does not exist", msg_tag_str("suite", suite_name), NULL);
      return FALSE;
    }

  test = _test_lookup_case(suite, test_name);
  if (!test)
    {
      log_error("Test does not exist in suite",
                msg_tag_str("suite", suite_name),
                msg_tag_str("case", test_name), NULL);
      return FALSE;
    }

  return _test_suite_run(self, suite, test);
}

gboolean
tinu_test_assert(gboolean condition, const gchar *assert_type, const gchar *condstr,
  const gchar *file, const gchar *func, gint line, MessageTag *tag0, ...)
{
  va_list vl;
  Message *msg;

  _test_run_hooks(TEST_HOOK_ASSERT, condition, file, func, line);

  if (!condition)
    g_test_case_current_result = TEST_FAILED;

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

  return condition;
}

const NameTable TestCaseResult_names[] =
{
  { TEST_NONE,        "none",       4 },
  { TEST_PASSED,      "passed",     6 },
  { TEST_FAILED,      "failed",     6 },
  { TEST_ABORT,       "abort",      5 },
  { TEST_SEGFAULT,    "segfault",   8 },
  { TEST_INTERNAL,    "internal",   8 },
  { 0,                NULL,         0 }
};

const NameTable TestHookID_names[] =
{
  { TEST_HOOK_ASSERT,           "TEST_HOOK_ASSERT",           16 },
  { TEST_HOOK_SIGNAL_ABORT,     "TEST_HOOK_SIGNAL_ABORT",     22 },
  { TEST_HOOK_SIGNAL_SEGFAULT,  "TEST_HOOK_SIGNAL_SEGFAULT",  25 },
  { TEST_HOOK_BEFORE_TEST,      "TEST_HOOK_BEFORE_TEST",      21 },
  { TEST_HOOK_AFTER_TEST,       "TEST_HOOK_AFTER_TEST",       20 },
  { TEST_HOOK_BEFORE_SUITE,     "TEST_HOOK_BEFORE_SUITE",     22 },
  { TEST_HOOK_AFTER_SUITE,      "TEST_HOOK_AFTER_SUITE",      21 },
  { TEST_HOOK_MAX,              "TEST_HOOK_MAX",              13 },
  { TEST_HOOK_ALL,              "TEST_HOOK_ALL",              13 },
  { 0,                          NULL,                          0 }
};

