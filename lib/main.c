#include <stdlib.h>
#include <string.h>
#include <signal.h>

#include <syslog.h>

#include <glib/goption.h>
#include <glib/gutils.h>

#include <tinu/log.h>
#include <tinu/leakwatch.h>

#include <tinu/main.h>

typedef struct _LeakInfo
{
  gboolean        m_enabled;
  GHashTable     *m_leaks;
  gpointer        m_watch_handle;
} LeakInfo;

typedef struct _MainTestContext
{
  TestContext     m_super;

  LeakInfo        m_leak_test;
  LeakInfo        m_leak_suite;
} MainTestContext;

static const TestContextFuncs g_main_test_context_funcs;
static GOptionEntry g_main_opt_entries[];

/* Options */
static gboolean g_opt_fancy = FALSE;
static gboolean g_opt_silent = FALSE;
static gboolean g_opt_syslog = FALSE;
static gint g_opt_priority = LOG_WARNING;

MainTestContext g_main_test_context;
static gboolean g_main_test_context_init = FALSE;

/* Error handling */
static inline GQuark
log_error_main()
{
  return g_quark_from_string("tinu-main-error");
}

enum
{
  MAIN_ERROR_OPTIONS
};

void
_tinu_signal_handler(int signo)
{
  log_error("Segmentation fault", msg_tag_trace_current("trace", 0), NULL);
  exit(1);
}

gboolean
_tinu_opt_leakwatch(const gchar *opt G_GNUC_UNUSED, const gchar *value,
  gpointer data, GError **error)
{
  if (!strcasecmp(value, "test"))
    {
      g_main_test_context.m_leak_test.m_enabled = TRUE;
    }
  else if (!strcasecmp(value, "suite"))
    {
      g_main_test_context.m_leak_suite.m_enabled = TRUE;
    }
  else if (!strcasecmp(value, "both"))
    {
      g_main_test_context.m_leak_test.m_enabled = TRUE;
      g_main_test_context.m_leak_suite.m_enabled = TRUE;
    }
  else
    {
      g_set_error(error, log_error_main(), MAIN_ERROR_OPTIONS,
                  "Unknown leakwatch option value `%s'", value);
      return FALSE;
    }

  return TRUE;
}

gboolean
_tinu_opt_priority(const gchar *opt G_GNUC_UNUSED, const gchar *value,
  gpointer data, GError **error)
{
  gint priority = msg_get_priority_value(value);

  if (priority == -1)
    {
      g_set_error(error, log_error_main(), MAIN_ERROR_OPTIONS,
                  "Unknown priority `%s'", value);
      return FALSE;
    }

  g_opt_priority = priority;
  return TRUE;
}

static GOptionEntry g_main_opt_entries[] = {
  { "fancy-log", 'c', 0, G_OPTION_ARG_NONE, (gpointer)&g_opt_fancy,
    "Colourfull logging to stderr", NULL },
  { "silent", 's', 0, G_OPTION_ARG_NONE, (gpointer)&g_opt_silent,
    "Do not log to stderr", NULL },
  { "syslog", 'S', 0, G_OPTION_ARG_NONE, (gpointer)&g_opt_syslog,
    "Log using standard syslog functions (with the 'user' facility", NULL },
  { "log-level", 'v', 0, G_OPTION_ARG_CALLBACK, (gpointer)&_tinu_opt_priority,
    "Set log priority (emergency, alert, critical, error, warning, notice, info, debug)",
    "level" },
  { "leak-watch", 'l', 0, G_OPTION_ARG_CALLBACK, (gpointer)&_tinu_opt_leakwatch,
    "Set leakwatch level (suite=Watch per suite; test=Watch per test; both=Both)",
    "watch-scope" },
  { NULL }
};

void
_tinu_start_leakwatch(LeakInfo *self)
{
  if (self->m_enabled)
    {
      t_assert(self->m_leaks == NULL);
      t_assert(self->m_watch_handle == NULL);
      self->m_watch_handle = tinu_leakwatch_simple(&self->m_leaks);
    }
}

void
_tinu_stop_leakwatch(LeakInfo *self)
{
  if (self->m_enabled)
    {
      tinu_unregister_watch(self->m_watch_handle);
      self->m_watch_handle = NULL;

      tinu_leakwatch_simple_dump(self->m_leaks, LOG_WARNING);
      g_hash_table_destroy(self->m_leaks);
      self->m_leaks = NULL;
    }
}

gboolean
_tinu_options(int *argc, char **argv[])
{
  GError *error = NULL;
  GOptionContext *context;
  gpointer watch;

  watch = tinu_register_message_handler(msg_stderr_handler, LOG_ERR, LOGMSG_PROPAGATE);

  context = g_option_context_new("Test runner");
  g_option_context_add_main_entries(context, g_main_opt_entries, NULL);
  if (!g_option_context_parse(context, argc, argv, &error))
    {
      log_error("Option parsing failed",
                msg_tag_str("message", error->message),
                msg_tag_str("domain", g_quark_to_string(error->domain)),
                msg_tag_int("code", error->code), NULL);
      g_error_free(error);
      return FALSE;
    }

  tinu_unregister_message_handler(watch);
  return TRUE;
}

int
tinu_main(int *argc, char **argv[])
{
  gint res;
  gchar *basename = g_path_get_basename(**argv);

  signal(SIGSEGV, _tinu_signal_handler);

  if (!_tinu_options(argc, argv))
    return 1;

  if (!g_opt_silent)
    {
      if (g_opt_fancy)
        tinu_register_message_handler(msg_stderr_fancy_handler, g_opt_priority, LOGMSG_PROPAGATE);
      else
        tinu_register_message_handler(msg_stderr_handler, g_opt_priority, LOGMSG_PROPAGATE);
    }

  if (g_opt_syslog)
    {
      openlog(basename, LOG_PID, LOG_USER);
      tinu_register_message_handler(msg_syslog_handler, g_opt_priority, LOGMSG_PROPAGATE);
    }

  res = tinu_test_all_run((TestContext *)&g_main_test_context) ? 0 : 1;

  test_context_destroy((TestContext *)&g_main_test_context);
  g_free(basename);

  return res;
}

void
tinu_test_add(const gchar *suite_name,
              const gchar *test_name,
              TestSetup setup,
              TestCleanup cleanup,
              TestFunction func)
{
  if (!g_main_test_context_init)
    {
      /* Initialize test context */
      memset(&g_main_test_context, 0, sizeof(MainTestContext));
      test_context_init((TestContext *)&g_main_test_context, &g_main_test_context_funcs);
      g_main_test_context_init = TRUE;
    }

  test_add((TestContext *)&g_main_test_context, suite_name, test_name, setup, cleanup, func);
}

gboolean
_test_prepare_suite(TestContext *context, TestSuite *suite)
{
  MainTestContext *self = (MainTestContext *)context;

  _tinu_start_leakwatch(&self->m_leak_suite);
  return TRUE;
}

void
_test_done_suite(TestContext *context, TestSuite *suite, gboolean res)
{
  MainTestContext *self = (MainTestContext *)context;

  _tinu_stop_leakwatch(&self->m_leak_suite);
}

gboolean
_test_prepare_test(TestContext *context, TestCase *test)
{
  MainTestContext *self = (MainTestContext *)context;

  _tinu_start_leakwatch(&self->m_leak_test);
  return TRUE;
}

void
_test_done_test(TestContext *context, TestCase *test, TestCaseResult res)
{
  MainTestContext *self = (MainTestContext *)context;

  _tinu_stop_leakwatch(&self->m_leak_test);
}

static const TestContextFuncs g_main_test_context_funcs = {
  _test_prepare_suite,
  _test_done_suite,
  _test_prepare_test,
  _test_done_test
};

