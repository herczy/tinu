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

#include <stdarg.h>
#include <unistd.h>

#include <errno.h>

#include <glib.h>

#include <tinu/reporting.h>
#include <tinu/log.h>

/* Options */
static const gchar *g_opt_program_name = NULL;
static const gchar *g_opt_program_template = "cat %s";

static const gchar *g_opt_filename = NULL;
static FILE *g_opt_file = NULL;

/* Globals */
static gchar g_prg_prefix[4096];

static void
_prg_report_set(const gchar *fmt, ...)
{
  va_list vl;

  va_start(vl, fmt);
  vsprintf(g_prg_prefix, fmt, vl);
  va_end(vl);
}

static void
_prg_report_print(FILE *file, const gchar *fmt, ...)
{
  va_list vl;

  va_start(vl, fmt);
  fprintf(file, "%s.", g_prg_prefix);
  vfprintf(file, fmt, vl);
  fprintf(file, "\n");
  va_end(vl);
}

static gchar *
_prg_report_replace(const gchar *template, const gchar *argument)
{
  const gchar *pos0;
  const gchar *pos;
  GString *string;

  string = g_string_sized_new(128);
  pos = pos0 = template;
  while (NULL != (pos = strstr(pos0, "%s")))
    {
      g_string_append_len(string, pos0, (gsize)(pos - pos0));
      g_string_append(string, argument);
      pos += 2;
      pos0 = pos;
    }

  g_string_append(string, pos0);
  return g_string_free(string, FALSE);
}

static FILE *
_prg_report_passfile(gchar **name)
{
  gchar tpl[] = "/tmp/tinu-pass-XXXXXX";
  gint fd = g_mkstemp(tpl);
  FILE *res = fdopen(fd, "w");

  g_assert(res);

  *name = g_strdup(tpl);
  return res;
}

static gboolean
test_report_program_check(StatisticsVerbosity verbosity, gboolean enable_colour)
{
  return TRUE;
}

static void
_test_report_put_file(FILE *file, TestStatistics *stat)
{
  gint i, j;
  double tics = (double)sysconf(_SC_CLK_TCK);

  StatSuiteInfo *suite;
  StatTestInfo *test;

  _prg_report_set("summary");
  _prg_report_print(file, "passed=%d", stat->m_passed);
  _prg_report_print(file, "failed=%d", stat->m_failed);
  _prg_report_print(file, "segfault=%d", stat->m_sigsegv);

  for (i = 0; i < stat->m_suite_info_list->len; i++)
    {
      suite = &g_array_index(stat->m_suite_info_list, StatSuiteInfo, i);

      _prg_report_set("suite.%s", suite->m_suite->m_name);
      _prg_report_print(file, "result=%d", suite->m_result ? 1 : 0);
      _prg_report_print(file, "asserts.passed=%d", suite->m_assertions_passed);
      _prg_report_print(file, "asserts.total=%d", suite->m_assertions);
      _prg_report_print(file, "time=%lf", (suite->m_end - suite->m_start) / tics);

      for (j = 0; j < suite->m_test_info_list->len; j++)
        {
          test = &g_array_index(suite->m_test_info_list, StatTestInfo, j);

          _prg_report_set("suite.%s.test.%s", suite->m_suite->m_name, test->m_test->m_name);
          _prg_report_print(file, "result=%s", test_result_name(test->m_result));
          _prg_report_print(file, "asserts.passed=%d", test->m_assertions_passed);
          _prg_report_print(file, "asserts.total=%d", test->m_assertions);
          _prg_report_print(file, "time=%lf", (test->m_end - test->m_start) / tics);
        }
    }

  _prg_report_set("message");
  for (i = LOG_CRIT; i <= LOG_DEBUG; i++)
    _prg_report_print(file, "%s=%d", msg_format_priority(i), stat->m_messages[i]);
}

static void
test_report_program(TestStatistics *stat, StatisticsVerbosity verbosity, gboolean enable_colour)
{
  gchar *fname;
  FILE *file = _prg_report_passfile(&fname);

  gchar *prog;

  _test_report_put_file(file, stat);

  fclose(file);

  if (g_opt_program_name)
    {
      prog = g_strdup_printf("%s %s", g_opt_program_name, fname);
    }
  else
    {
      prog = _prg_report_replace(g_opt_program_template, fname);
    }

  log_debug("Running statistics",
            msg_tag_str("program", g_opt_program_name),
            msg_tag_str("command", prog),
            NULL);

  system(prog);
  unlink(fname);
  g_free(fname);
  g_free(prog);
}

const GOptionEntry g_report_program_module_options[] = {
  { "program", 0, 0, G_OPTION_ARG_STRING, (gpointer)&g_opt_program_name,
    "The program to pass the statistics to. The file name is passed as an argument to the program. "
    "To pass extra options, use program-template.", NULL },
  { "program-template", 'T', 0, G_OPTION_ARG_STRING, (gpointer)&g_opt_program_template,
    "Use a program template. All occurrences of %s are replaced with the filename "
    "of the statistics (default 'cat %s')", NULL },
  { NULL }
};

const ReportModule g_report_progam_module = {
  .m_name = "program",
  .m_options = g_report_program_module_options,

  .m_check = &test_report_program_check,
  .m_handle = &test_report_program
};

static gboolean
test_report_file_check(StatisticsVerbosity verbosity, gboolean enable_colour)
{
  gint _errno;

  if (!g_opt_filename)
    {
      log_error("Missing destination name from file report", NULL);
      return FALSE;
    }
  g_opt_file = fopen(g_opt_filename, "w");
  if (!g_opt_file)
    {
      _errno = errno;
      log_error("Cannot create file",
                msg_tag_str("name", g_opt_filename),
                msg_tag_int("code", _errno),
                msg_tag_str("reason", strerror(_errno)), NULL);
      return FALSE;
    }
  return TRUE;
}

static void
test_report_file(TestStatistics *stat, StatisticsVerbosity verbosity, gboolean enable_colour)
{
  _test_report_put_file(g_opt_file, stat);
  fclose(g_opt_file);
}

const GOptionEntry g_report_file_module_options[] = {
  { "file-destination", 0, 0, G_OPTION_ARG_STRING, (gpointer)&g_opt_filename,
    "Destination file", NULL },
  { NULL }
};

const ReportModule g_report_file_module = {
  .m_name = "file",
  .m_options = g_report_file_module_options,

  .m_check = &test_report_file_check,
  .m_handle = &test_report_file
};

