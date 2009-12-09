#include <stdlib.h>
#include <string.h>

#include <ucontext.h>
#include <signal.h>

#include <tinu/utils.h>
#include <tinu/test.h>
#include <tinu/backtrace.h>
#include <tinu/leakwatch.h>

#include <config.h>

#ifndef sighandler_t
typedef void (*sighandler_t)(int);
#endif

static gint g_signal = 0;

static sighandler_t g_sigsegv_handler = NULL;
static sighandler_t g_sigabrt_handler = NULL;

static TestCase *g_test_case_current = NULL;

ucontext_t g_test_context;

typedef struct _LeakInfo
{
  gboolean        m_enabled;
  GHashTable     *m_leaks;
  gpointer        m_watch_handle;
} LeakInfo;

void
_signal_handler(int signo)
{
  Backtrace *trace = backtrace_create(3);
  log_error("Signal received while running a test",
            msg_tag_int("signal", signo),
            msg_tag_str("suite", g_test_case_current->m_suite->m_name),
            msg_tag_str("test", g_test_case_current->m_name), NULL);
  backtrace_dump_log(trace, "    ", LOG_ERR);
  backtrace_unreference(trace);

  g_signal = signo;

  setcontext(g_test_context.uc_link);
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

gboolean
_test_message_counter(Message *msg, gpointer user_data)
{
  TestContext *self = (TestContext *)user_data;
  self->m_statistics.m_messages[msg->m_priority]++;
  return TRUE;
}

void
_test_case_run_intern(TestContext *self, TestCase *test, TestCaseResult *result)
{
  gpointer ctx = NULL;

  g_test_case_current = test;

  if (test->m_setup && !test->m_setup(&ctx))
    {
      *result = TEST_FAILED;
      return;
    }

  if (test->m_test)
    test->m_test(ctx);

  if (test->m_cleanup)
    test->m_cleanup(ctx);

  *result = TEST_PASSED;
}

TestCaseResult
_test_case_run(TestContext *self, TestCase *test)
{
  TestCaseResult res;
  _test_case_run_intern(self, test, &res);
  return res;
}

TestCaseResult
_test_case_run_single_test(TestContext *self, TestCase *test)
{
  TestCaseResult res = TEST_PASSED;
  gpointer log_handler = log_register_message_handler(_test_message_counter,
                                                      LOG_DEBUG, (gpointer)self);

  GHashTable *leak_table = NULL;
  gpointer leak_handler = (self->m_leakwatch ? tinu_leakwatch_simple(&leak_table) : NULL);

  gpointer stack = g_malloc0(TEST_CTX_STACK_SIZE);

  ucontext_t main_ctx;

  if (self->m_sighandle)
    {
      _signal_on();

      if (getcontext(&g_test_context) == -1)
        {
          log_error("Cannot get main context", msg_tag_errno(), NULL);
          goto internal_error;
        }

      g_test_context.uc_stack.ss_sp = stack;
      g_test_context.uc_stack.ss_size = TEST_CTX_STACK_SIZE;
      g_test_context.uc_link = &main_ctx;
      makecontext(&g_test_context, (void (*)())(&_test_case_run_intern), 3, self, test, &res);
    
      if (swapcontext(&main_ctx, &g_test_context) == -1)
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
          self->m_statistics.m_passed++;

          log_notice("Test case run successfull",
                     msg_tag_str("case", test->m_name),
                     msg_tag_str("suite", test->m_suite->m_name), NULL);
        }
      else
        {
          self->m_statistics.m_failed++;

          log_warn("Test case run failed",
                   msg_tag_str("case", test->m_name),
                   msg_tag_str("suite", test->m_suite->m_name), NULL);
        }
    }
  else
    {
      if (g_signal == SIGSEGV)
        {
          res = TEST_SEGFAULT;
          self->m_statistics.m_sigsegv++;
        }
      else
        {
          res = TEST_FAILED;
          self->m_statistics.m_failed++;
        }

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

  test->m_result = res;

  log_unregister_message_handler(log_handler);
  return res;

internal_error:
  test->m_result = TEST_INTERNAL;
  self->m_statistics.m_failed++;
  return TEST_INTERNAL;
}

gboolean
_test_suite_run(TestContext *self, TestSuite *suite)
{
  gint i;
  gboolean res = TRUE;

  for (i = 0; i < suite->m_tests->len; i++)
    res &= TEST_PASSED ==
      _test_case_run_single_test(self, (TestCase *)g_ptr_array_index(suite->m_tests, i));

  log_format(res ? LOG_DEBUG : LOG_WARNING, "Test suite run complete",
            msg_tag_str("suite", suite->m_name),
            msg_tag_bool("result", res), NULL);

  suite->m_passed = res;
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
}

void
test_context_destroy(TestContext *self)
{
  g_ptr_array_free(self->m_suites, TRUE);
}

void
test_add(TestContext *self,
         const gchar *suite_name,
         const gchar *test_name,
         TestSetup setup,
         TestCleanup cleanup,
         TestFunction func)
{
  TestSuite *suite = _test_suite_lookup(self, suite_name, TRUE);
  TestCase *res = _test_lookup_case(suite, test_name);

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

