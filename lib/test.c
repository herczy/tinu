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

void
_signal_handler(int signo)
{
  /* fprintf(stderr, "Signal: %d\n", signo); */
  g_signal = signo;
  siglongjmp(g_jump, 1);
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

gboolean
_test_case_run(TestContext *self, TestCase *test)
{
  gpointer ctx = NULL;
  gboolean res = TRUE;

  if (self->m_prepare_test &&
      !self->m_prepare_test(self, test))
    {
      log_error("Could not prepare test case running",
                msg_tag_str("case", test->m_name),
                msg_tag_str("suite", test->m_suite->m_name), NULL);
      return -1;
    }

  if (test->m_setup)
    res = test->m_setup(&ctx);

  if (res && test->m_test)
    res = test->m_test(ctx);

  if (test->m_cleanup)
    test->m_cleanup(ctx);

  if (self->m_done_test)
    self->m_done_test(self, test, res);

  tinu_plog(res ? LOG_NOTICE : LOG_WARNING, "Test case run complete",
            msg_tag_str("case", test->m_name),
            msg_tag_str("suite", test->m_suite->m_name),
            msg_tag_bool("result", res), NULL);

  return res;
}

gboolean
_test_case_run_isolated(TestContext *self, TestCase *test)
{
  gboolean res = TRUE;

  _signal_on();

  if (sigsetjmp(g_jump, 1) > 0)
    {
      log_error("Signal received", msg_tag_int("signal", g_signal), NULL);
      res = FALSE;
    }
  else
    {
      res = _test_case_run(self, test);
    }

  _signal_off();

  return res;
}

gboolean
_test_suite_run(TestContext *self, TestSuite *suite)
{
  gint i;
  gboolean res = TRUE;

  if (self->m_prepare_suite &&
      !self->m_prepare_suite(self, suite))
    {
      log_error("Suite prepare failed",
                msg_tag_str("suite", suite->m_name), NULL);
      return FALSE;
    }

  for (i = 0; i < suite->m_tests->len; i++)
    {
      res &= _test_case_run_isolated(self, (TestCase *)g_ptr_array_index(suite->m_tests, i));
    }

  if (self->m_done_suite)
    self->m_done_suite(self, suite, res);

  tinu_plog(res ? LOG_DEBUG : LOG_WARNING, "Test suite run complete",
            msg_tag_str("suite", suite->m_name),
            msg_tag_bool("result", res), NULL);
  return res;
}

static TestSuite *
_test_suite_lookup_new(TestContext *self, const gchar *suite)
{
  guint i;
  TestSuite *res = NULL;

  for (i = 0; i < self->m_suites->len; i++)
    {
      res = (TestSuite *)g_ptr_array_index(self->m_suites, i);
      if (!strcmp(res->m_name, suite))
        goto exit;
    }

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
test_context_init(TestContext *self, const TestContextFuncs *funcs)
{
  self->m_prepare_suite = funcs->m_prepare_suite;
  self->m_done_suite = funcs->m_done_suite;
  self->m_prepare_test = funcs->m_prepare_test;
  self->m_done_test = funcs->m_done_test;

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
  TestSuite *suite = _test_suite_lookup_new(self, suite_name);
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

