/* TINU - Unittesting framework
*
* Copyright (c) 2010, Viktor Hercinger <hercinger.viktor@gmail.com>
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

#include <tinu/statistics.h>
#include <tinu/reporting.h>

#include <glib.h>

#define SIZE_1KB 1024
#define SIZE_1MB SIZE_1KB * 1024

static gboolean g_opt_stderr = FALSE;
static FILE *g_opt_print_out;

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

  vfprintf(g_opt_print_out, fmt->str, vl);
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

static char *
_humanly_readable_size(char *dest, int max_size, gsize size)
{
  if (size >= SIZE_1MB)
    snprintf(dest, max_size, "%.2lf MB", (double)size / (double)SIZE_1MB);
  else if (size >= SIZE_1KB)
    snprintf(dest, max_size, "%.2lf kB", (double)size / (double)SIZE_1KB);
  else
    snprintf(dest, max_size, "%u B", size);

  return dest;
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
      fprintf(g_opt_print_out, " assertions passed: %d/%d",
        suite->m_assertions_passed, suite->m_assertions);
    }
  fprintf(g_opt_print_out, "\n");
}

void
_std_report_show_case(StatTestInfo *test, StatisticsVerbosity verbosity, gboolean colour)
{
  char leaked_mem_str[128];
  const gchar *result_name = test_result_name(test->m_result);
  _report_printf("   Case ", COL_ENTRY("%-*s"), NULL, 33, test->m_test->m_name);

  switch (test->m_result)
    {
      case TEST_PASSED :
        _report_printf(COL_OK("%s"), NULL, result_name);
        break;

      case TEST_FAILED :
        _report_printf(COL_FAIL("%s"), NULL, result_name);
        break;

      case TEST_SEGFAULT :
      case TEST_ABORT :
        _report_printf(COL_FATAL("%s"), NULL, result_name);
#ifdef COREDUMPER_ENABLED
        fprintf(g_opt_print_out, " (core: %s)",
          core_file_name(g_opt_core_dir, test->m_suite->m_name, test->m_name));
#endif
        break;

      case TEST_INTERNAL :
        _report_printf(COL_FATAL("%s"), NULL, result_name);
        break;

      default :
        g_assert_not_reached();
    }

  if (verbosity == STAT_VERB_VERBOSE)
    {
      fprintf(g_opt_print_out, " assertions passed: %d/%d",
        test->m_assertions_passed, test->m_assertions);
    }

  if (test->m_leaked_bytes)
    {
      fprintf(g_opt_print_out, " leaked: %s",
        _humanly_readable_size(leaked_mem_str,
                               sizeof(leaked_mem_str),
                               test->m_leaked_bytes));
    }
  fprintf(g_opt_print_out, "\n");
}

static gboolean
test_report_print_check(StatisticsVerbosity verbosity, gboolean enable_colour)
{
  g_opt_print_out = (g_opt_stderr ? stderr : stdout);
  return TRUE;
}

static void
test_report_print(TestStatistics *stat, StatisticsVerbosity verbosity, gboolean enable_colour)
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
          fprintf(g_opt_print_out, "\n");
        }
    }

  if (verbosity <= STAT_VERB_FULL)
    return;

  fprintf(g_opt_print_out, "Messages received:\n");
  for (i = LOG_CRIT; i <= LOG_DEBUG; i++)
    {
      fprintf(g_opt_print_out, "    %-*s %*d\n",
              25, msg_format_priority(i),
              10, stat->m_messages[i]);
    }
}

const GOptionEntry g_report_print_module_options[] = {
  { "stderr", 0, 0, G_OPTION_ARG_NONE, (gpointer)&g_opt_stderr,
    "Print messages to stderr instead of stderr", NULL },
  { NULL }
};

const ReportModule g_report_print_module = {
  .m_name = "print",
  .m_options = g_report_print_module_options,
  .m_check = &test_report_print_check,
  .m_handle = &test_report_print
};
