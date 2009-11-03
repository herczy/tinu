#include <stdlib.h>
#include <string.h>

#include <setjmp.h>
#include <signal.h>

#include <tinu/utils.h>
#include <tinu/test.h>
#include <tinu/backtrace.h>
#include <tinu/leakwatch.h>

#ifndef sighandler_t
typedef void (*sighandler_t)(int);
#endif

static gint g_signal = 0;

static sighandler_t g_sigsegv_handler = NULL;
static sighandler_t g_sigabrt_handler = NULL;

jmp_buf g_jump;

typedef struct _LeakInfo
{
  gboolean        m_enabled;
  GHashTable     *m_leaks;
  gpointer        m_watch_handle;
} LeakInfo;

void
_signal_handler(int signo)
{
  log_error("Signal received while running a test",
            msg_tag_int("signal", signo),
            msg_tag_trace_current("trace", 3), NULL);

  g_signal = signo;
  siglongjmp(g_jump, 1);
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

TestCaseResult
_test_case_run(TestContext *self, TestCase *test)
{
  gpointer ctx = NULL;

  if (test->m_setup && !test->m_setup(&ctx))
    return TEST_FAILED;

  if (test->m_test)
    test->m_test(ctx);

  if (test->m_cleanup)
    test->m_cleanup(ctx);

  return TEST_PASSED;
}

TestCaseResult
_test_case_run_sighnd(TestContext *self, TestCase *test)
{
  TestCaseResult res = TEST_PASSED;
  gpointer log_handler = log_register_message_handler(_test_message_counter,
                                                      LOG_DEBUG, (gpointer)self);

  GHashTable *leak_table = NULL;
  gpointer leak_handler = (self->m_leakwatch ? tinu_leakwatch_simple(&leak_table) : NULL);

  _signal_on();

  if (sigsetjmp(g_jump, 1) > 0)
    {
      if (g_signal == SIGSEGV)
        {
          self->m_statistics.m_sigsegv++;
          res = TEST_SEGFAULT;
        }
      else
        {
          res = TEST_FAILED;
        }

      log_format(g_signal == SIGABRT ? LOG_WARNING : LOG_ERR, "Test case run returned with signal",
                 msg_tag_int("signal", g_signal),
                 msg_tag_str("case", test->m_name),
                 msg_tag_str("suite", test->m_suite->m_name), NULL);

      if (leak_handler)
        {
          tinu_unregister_watch(leak_handler);
          g_hash_table_destroy(leak_table);
        }
    }
  else
    {
      res = _test_case_run(self, test);

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

      if (leak_handler)
        {
          tinu_unregister_watch(leak_handler);
          tinu_leakwatch_simple_dump(leak_table, LOG_WARNING);
          g_hash_table_destroy(leak_table);
        }
    }

  _signal_off();

  if (res == TEST_PASSED)
    {
      self->m_statistics.m_passed++;
    }
  else
    {
      self->m_statistics.m_failed++;
    }

  test->m_result = res;

  log_unregister_message_handler(log_handler);
  return res;
}

gboolean
_test_suite_run(TestContext *self, TestSuite *suite)
{
  gint i;
  gboolean res = TRUE;

  for (i = 0; i < suite->m_tests->len; i++)
    res &= TEST_PASSED == (self->m_sighandle ? _test_case_run_sighnd : _test_case_run)
                          (self, (TestCase *)g_ptr_array_index(suite->m_tests, i));

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

