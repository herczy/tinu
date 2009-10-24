#include <stdlib.h>
#include <string.h>
#include <signal.h>

#include <applog.h>

#include <glib/goption.h>
#include <glib/gutils.h>

#include <tinu/leakwatch.h>
#include <tinu/main.h>

#include <config.h>

typedef enum
{
  STAT_VERB_NONE = 0,
  STAT_VERB_SUMMARY,
  STAT_VERB_SUITES,
  STAT_VERB_FULL,
  STAT_VERB_VERBOSE,
} StatisticsVerbosity;

//static const TestContextFuncs g_main_test_context_funcs;
static GOptionEntry g_main_opt_entries[];

static gboolean g_main_test_context_init = FALSE;
static TestContext g_main_test_context;

/* Options */
static gboolean g_opt_fancy = FALSE;
static gboolean g_opt_silent = FALSE;
static gboolean g_opt_syslog = FALSE;
static gboolean g_opt_sighandle = TRUE;
static gboolean g_opt_leakwatch = FALSE;
static gboolean g_opt_version = FALSE;
static gint g_opt_priority = LOG_WARNING;
static StatisticsVerbosity g_opt_stat_verb = STAT_VERB_SUMMARY;

static const gchar *g_opt_suite = NULL;

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
  signal(SIGSEGV, SIG_DFL);
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

gboolean
_tinu_opt_stat_verb(const gchar *opt G_GNUC_UNUSED, const gchar *value,
  gpointer data, GError **error)
{
  if (!strcasecmp(value, "none"))
    {
      g_opt_stat_verb = STAT_VERB_NONE;
    }
  else if (!strcasecmp(value, "summary"))
    {
      g_opt_stat_verb = STAT_VERB_SUMMARY;
    }
  else if (!strcasecmp(value, "suites"))
    {
      g_opt_stat_verb = STAT_VERB_SUITES;
    }
  else if (!strcasecmp(value, "full"))
    {
      g_opt_stat_verb = STAT_VERB_FULL;
    }
  else if (!strcasecmp(value, "verbose"))
    {
      g_opt_stat_verb = STAT_VERB_VERBOSE;
    }
  else
    {
      g_set_error(error, log_error_main(), MAIN_ERROR_OPTIONS,
                  "Unknown statistics verbosity `%s'", value);
      return FALSE;
    }

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
  { "results", 'R', 0, G_OPTION_ARG_CALLBACK, (gpointer)&_tinu_opt_stat_verb,
    "Set statistics verbosity (none, summary (default), suites, full, verbose)",
    "verbosity" },
  { "leakwatch", 0, 0, G_OPTION_ARG_NONE, (gpointer)&g_opt_leakwatch,
    "Enable leakwatcher (warning: slows tests down by a significant ammount of time)", NULL },
  { "no-sighandle", 0, G_OPTION_FLAG_HIDDEN | G_OPTION_FLAG_REVERSE, G_OPTION_ARG_NONE, 
    (gpointer)&g_opt_sighandle,
    "Don't handle signals from test", NULL },
  { "suite", 0, 0, G_OPTION_ARG_STRING, (gpointer)&g_opt_suite,
    "Run only the given suite", NULL },
  { "version", 'V', 0, G_OPTION_ARG_NONE, (gpointer)&g_opt_version,
    "Print version", NULL },
  { NULL }
};

static void
_tinu_version()
{
  fprintf(stderr, APPNAME " version " VERSION "\n"
                  "Build time: " BUILDTIME "\n"
#ifdef GIT_BRANCH
                  "Git branch: " GIT_BRANCH "\n"
#endif
#ifdef GIT_COMMIT
                  "Git commit ID: " GIT_COMMIT "\n"
#endif
                  );
  exit(1);
}

gboolean
_tinu_options(int *argc, char **argv[])
{
  GError *error = NULL;
  GOptionContext *context;
  gpointer watch;

  watch = log_register_message_handler(msg_stderr_handler, LOG_ERR, LOGMSG_PROPAGATE);

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

  log_unregister_message_handler(watch);
  return TRUE;
}

void
_tinu_show_summary(TestStatistics *stat)
{
  if (g_opt_fancy)
    {
      if (stat->m_sigsegv)
        {
          fprintf(stderr, "Summary: "
                          "\033[1;32mpassed: %d\033[0m; "
                          "\033[1;31mfailed: %d\033[0m; "
                          "\033[41m\033[1;37msegmentation faults: %d\033[0m\n",
                  stat->m_passed, stat->m_failed, stat->m_sigsegv);
        }
      else
        {
          fprintf(stderr, "Summary: "
                          "\033[1;32mpassed: %d\033[0m; "
                          "\033[1;31mfailed: %d\033[0m\n",
                  stat->m_passed, stat->m_failed);
        }
    }
  else
    fprintf(stderr, "Summary: passed: %d; failed: %d; segmentation faults: %d\n",
            stat->m_passed, stat->m_failed, stat->m_sigsegv);
}

void
_tinu_show_suite(TestSuite *suite)
{
  if (g_opt_fancy)
    {
      fprintf(stderr, "Suite   \033[35m%-*s\033[0m %s\n",
              30, suite->m_name,
              suite->m_passed ? "\033[32mpassed\033[0m" : "\033[31mfailed\033[0m");
    }
  else
    {
      fprintf(stderr, "Suite   %-*s %s\n",
              30, suite->m_name,
              suite->m_passed ? "passed" : "failed");
    }
}

void
_tinu_show_case(TestCase *test)
{
  int len;

  if (g_opt_fancy)
    {
      len = fprintf(stderr, "   Case \033[35m%-*s\033[0m ", 33, test->m_name);

      switch (test->m_result)
        {
          case TEST_PASSED :
            fprintf(stderr, "\033[32mpassed\033[0m\n");
            break;

          case TEST_FAILED :
            fprintf(stderr, "\033[31mfailed\033[0m\n");
            break;

          case TEST_SEGFAULT :
            fprintf(stderr, "\033[41m\033[1;37msegfault\033[0m\n");
            break;
        }

    }
  else
    {
      fprintf(stderr, "   Case %-*s ", 33, test->m_name);

      switch (test->m_result)
        {
          case TEST_PASSED :
            fprintf(stderr, "passed\n");
            break;

          case TEST_FAILED :
            fprintf(stderr, "failed\n");
            break;

          case TEST_SEGFAULT :
            fprintf(stderr, "segfault\n");
            break;
        }
    }
}

void
_tinu_show_results()
{
  gint i, j;

  TestSuite *suite;
  TestCase *test;

  if (g_opt_stat_verb == STAT_VERB_NONE)
    return;

  _tinu_show_summary(&g_main_test_context.m_statistics);

  if (g_opt_stat_verb == STAT_VERB_SUMMARY)
    return;

  for (i = 0; i < g_main_test_context.m_suites->len; i++)
    {
      suite = g_ptr_array_index(g_main_test_context.m_suites, i);

      if (g_opt_suite && strcmp(g_opt_suite, suite->m_name) != 0)
        continue;

      _tinu_show_suite(suite);

      if (g_opt_stat_verb > STAT_VERB_SUITES)
        {
          for (j = 0; j < suite->m_tests->len; j++)
            _tinu_show_case(g_ptr_array_index(suite->m_tests, j));
          fprintf(stderr, "\n");
        }

      if (g_opt_suite)
        break;
    }

  if (g_opt_stat_verb <= STAT_VERB_FULL)
    return;

  fprintf(stderr, "Messages received:\n");
  for (i = LOG_CRIT; i <= LOG_DEBUG; i++)
    {
      fprintf(stderr, "    %-*s %*d\n",
              25, msg_format_priority(i),
              10, g_main_test_context.m_statistics.m_messages[i]);
    }
}

void
_tinu_log_clear(gpointer user_data G_GNUC_UNUSED)
{
  log_clear();
}

int
tinu_main(int *argc, char **argv[])
{
  gboolean res;
  gchar *basename = g_path_get_basename(**argv);

  signal(SIGSEGV, _tinu_signal_handler);

  if (!_tinu_options(argc, argv))
    return 1;

  if (g_opt_version)
    _tinu_version();

  tinu_atexit(_tinu_log_clear);

  if (!g_opt_silent)
    {
      if (g_opt_fancy)
        log_register_message_handler(msg_stderr_fancy_handler, g_opt_priority, LOGMSG_PROPAGATE);
      else
        log_register_message_handler(msg_stderr_handler, g_opt_priority, LOGMSG_PROPAGATE);
    }

  if (g_opt_syslog)
    {
      openlog(basename, LOG_PID, LOG_USER);
      log_register_message_handler(msg_syslog_handler, g_opt_priority, LOGMSG_PROPAGATE);
    }

  g_main_test_context.m_sighandle = g_opt_sighandle;
  g_main_test_context.m_leakwatch = g_opt_leakwatch;

  log_debug("Running tests", NULL);
  if (g_opt_suite)
    res = tinu_test_suite_run(&g_main_test_context, g_opt_suite);
  else
    res = tinu_test_all_run(&g_main_test_context);

  _tinu_show_results();

  test_context_destroy(&g_main_test_context);
  g_free(basename);

  return res ? 0 : 1;
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
      test_context_init((TestContext *)&g_main_test_context);
      g_main_test_context_init = TRUE;
    }

  test_add((TestContext *)&g_main_test_context, suite_name, test_name, setup, cleanup, func);
}
