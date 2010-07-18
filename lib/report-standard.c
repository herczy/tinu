#include <tinu/statistics.h>
#include <tinu/reporting.h>

#include <glib/gstring.h>

static void
_report_vprintf(const gchar *fmt0, va_list vl)
{
  const gchar *fmt_cur = fmt0;
  GString *fmt = g_string_new(NULL);

  while (fmt_cur)
    {
      g_string_append(fmt, fmt_cur);
      fmt_cur = va_arg(vl, const gchar *);
    }

  vfprintf(stderr, fmt->str, vl);
  g_string_free(fmt, TRUE);
}

static void
_report_printf(const gchar *fmt0, ...)
{
  va_list vl;

  va_start(vl, fmt0);
  _report_vprintf(fmt0, vl);
  va_end(vl);
}

#define COL_OK(str) (colour ? "\033[32m" str "\033[0m" : str)
#define COL_FAIL(str) (colour ? "\033[31m" str "\033[0m" : str)
#define COL_FATAL(str) (colour ? "\033[1;41m" str "\033[0m" : str)
#define COL_ENTRY(str) (colour ? "\033[35m" str "\033[0m" : str)

static inline void
_std_report_show_summary(TestStatistics *stat, gboolean colour)
{
  _report_printf("Summary: ",
                 COL_OK("passed: %d "),
                 COL_FAIL("failed: %d "),
                 (stat->m_sigsegv ? COL_FATAL("segmentation faults: %d") : ""),
                 "\n", NULL,
                 stat->m_passed, stat->m_failed, stat->m_sigsegv);
}

static inline void
_std_report_show_suite(StatSuiteInfo *suite, StatisticsVerbosity verbosity, gboolean colour)
{
  _report_printf("Suite  ", COL_ENTRY("%-*s"), " %s", NULL,
    30, suite->m_suite->m_name, suite->m_result ? COL_OK("passed") : COL_FAIL("failed"));

  if (verbosity == STAT_VERB_VERBOSE)
    {
      fprintf(stderr, " assertions passed: %d/%d",
        suite->m_assertions_passed, suite->m_assertions);
    }
  fprintf(stderr, "\n");
}

void
_std_report_show_case(StatTestInfo *test, StatisticsVerbosity verbosity, gboolean colour)
{
  _report_printf("   Case ", COL_ENTRY("%-*s"), NULL, 33, test->m_test->m_name);

  switch (test->m_result)
    {
      case TEST_PASSED :
        _report_printf(COL_OK("passed"), NULL);
        break;

      case TEST_FAILED :
        _report_printf(COL_FAIL("failed"), NULL);
#ifdef COREDUMPER_ENABLED
        fprintf(stderr, " (core: %s)",
          core_file_name(g_opt_core_dir, test->m_suite->m_name, test->m_name));
#endif
        break;

      case TEST_SEGFAULT :
        _report_printf(COL_FATAL("segfault"), NULL);
#ifdef COREDUMPER_ENABLED
        fprintf(stderr, " (core: %s)",
          core_file_name(g_opt_core_dir, test->m_suite->m_name, test->m_name));
#endif
        break;

      case TEST_INTERNAL :
        _report_printf(COL_FATAL("internal error"));
        break;
    }

  if (verbosity == STAT_VERB_VERBOSE)
    {
      fprintf(stderr, " assertions passed: %d/%d",
        test->m_assertions_passed, test->m_assertions);
    }
  fprintf(stderr, "\n");
}

static void
test_report_stderr(TestStatistics *stat, StatisticsVerbosity verbosity, gboolean enable_colour)
{
  gint i, j;

  StatSuiteInfo *suite;

  _std_report_show_summary(stat, enable_colour);

  if (verbosity == STAT_VERB_SUMMARY)
    return;

  for (i = 0; i < stat->m_suite_info_list->len; i++)
    {
      suite = &g_array_index(stat->m_suite_info_list, StatSuiteInfo, i);
      _std_report_show_suite(suite, verbosity, enable_colour);

      if (verbosity > STAT_VERB_SUITES)
        {
          for (j = 0; j < suite->m_test_info_list->len; j++)
            _std_report_show_case(&g_array_index(suite->m_test_info_list, StatTestInfo, j),
              verbosity, enable_colour);
          fprintf(stderr, "\n");
        }
    }

  if (verbosity <= STAT_VERB_FULL)
    return;

  fprintf(stderr, "Messages received:\n");
  for (i = LOG_CRIT; i <= LOG_DEBUG; i++)
    {
      fprintf(stderr, "    %-*s %*d\n",
              25, msg_format_priority(i),
              10, stat->m_messages[i]);
    }
}

const ReportModule g_report_stderr_module = {
  .m_name = "stderr",
  .m_options = NULL,
  .m_check = NULL,
  .m_handle = &test_report_stderr
};
