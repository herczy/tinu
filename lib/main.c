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
#include <signal.h>

#include <glib/gutils.h>

#include <tinu/config.h>
#include <tinu/utils.h>
#include <tinu/leakwatch.h>
#include <tinu/main.h>
#include <tinu/log.h>
#include <tinu/clist.h>
#include <tinu/reporting.h>

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
static const gchar *g_opt_file = NULL;

static const gchar *g_opt_report = "print";

#ifdef COREDUMPER_ENABLED
static const gchar *g_opt_core_dir = "/tmp";
#endif

/* Runtime name */
const gchar *g_runtime_name = NULL;

/* Reporting modules defined in the tinu framework */
extern const ReportModule g_report_print_module;
extern const ReportModule g_report_progam_module;
extern const ReportModule g_report_file_module;

/* Report module structure */
static CList *g_report_modules = NULL;

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
  gchar *endl;

  if (priority == -1)
    {
      priority = strtoul(value, &endl, 10);
      if (!endl || (*endl) != '\0' || priority < LOG_EMERG || priority > LOG_DEBUG)
        priority = -1;
    }

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
  NameTableKey key = tinu_lookup_name(StatisticsVerbosity_names, value, -1, -1);
  gchar *endl;

  if (key == -1)
    {
      key = strtoul(value, &endl, 10);
      if (!endl || (*endl) != '\0' || key < STAT_VERB_NONE || key > STAT_VERB_VERBOSE)
        key = -1;
    }

  if (key == -1)
    {
      g_set_error(error, log_error_main(), MAIN_ERROR_OPTIONS,
                  "Unknown statistics verbosity `%s'", value);
      return FALSE;
    }

  g_opt_stat_verb = key;
  return TRUE;
}

gboolean
_tinu_opt_report_null(const gchar *opt G_GNUC_UNUSED, const gchar *value G_GNUC_UNUSED,
  gpointer data, GError **error)
{
  g_opt_report = NULL;
  return TRUE;
}

static GOptionEntry g_main_opt_entries[] = {
  { "fancy-log", 'c', 0, G_OPTION_ARG_NONE, (gpointer)&g_opt_fancy,
    "Colourful logging to stderr", NULL },
  { "silent", 's', 0, G_OPTION_ARG_NONE, (gpointer)&g_opt_silent,
    "Do not log to stderr", NULL },
  { "syslog", 'S', 0, G_OPTION_ARG_NONE, (gpointer)&g_opt_syslog,
    "Log using standard syslog functions (with the 'user' facility", NULL },
  { "file", 'f', 0, G_OPTION_ARG_STRING, (gpointer)&g_opt_file,
    "Log into a file", NULL },
  { "log-level", 'v', 0, G_OPTION_ARG_CALLBACK, (gpointer)&_tinu_opt_priority,
    "Set log priority (emergency, alert, critical, error, warning, notice, info, debug)",
    "level" },
  { "results", 'R', 0, G_OPTION_ARG_CALLBACK, (gpointer)&_tinu_opt_stat_verb,
    "Set statistics verbosity (none, summary (default), suites, full, verbose or 0 - 7)",
    "verbosity" },
  { "leakwatch", 0, 0, G_OPTION_ARG_NONE, (gpointer)&g_opt_leakwatch,
    "Enable leak watcher (warning: slows tests down by a significant ammount of time)", NULL },
  { "no-sighandle", 0, G_OPTION_FLAG_HIDDEN | G_OPTION_FLAG_REVERSE, G_OPTION_ARG_NONE, 
    (gpointer)&g_opt_sighandle,
    "Don't handle signals from test", NULL },
  { "suite", 0, 0, G_OPTION_ARG_STRING, (gpointer)&g_opt_suite,
    "Run only the given suite", NULL },
  { "report", 0, 0, G_OPTION_ARG_STRING, (gpointer)&g_opt_report,
    "Use the given report module (default: print)", NULL },
  { "no-report", 0, G_OPTION_ARG_NONE, G_OPTION_ARG_CALLBACK, (gpointer)&_tinu_opt_report_null,
    "Disable reporting", NULL },
#ifdef COREDUMPER_ENABLED
  { "core-dir", 0, 0, G_OPTION_ARG_STRING, (gpointer)&g_opt_core_dir,
    "Set target core directory (default: /tmp)" },
#endif
  { "version", 'V', 0, G_OPTION_ARG_NONE, (gpointer)&g_opt_version,
    "Print version", NULL },
  { NULL }
};

static void
_tinu_version()
{
  fprintf(stderr, PACKAGE_NAME " version " PACKAGE_VERSION "\n"
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

static gboolean
_tinu_options(int *argc, char **argv[])
{
  GError *error = NULL;
  GOptionContext *context;

  CListIterator *iter;
  ReportModule *mod;
  char description[4096];
  GOptionGroup *grp;

  context = g_option_context_new("Test runner");
  g_option_context_add_main_entries(context, g_main_opt_entries, NULL);

  for (iter = clist_iter_new(g_report_modules); clist_iter_next(iter); )
    {
      mod = (ReportModule *)clist_iter_data(iter);

      snprintf(description, sizeof(description), "`%s' reporting module", mod->m_name);
      grp = g_option_group_new(mod->m_name, description, description, NULL, NULL);
      if (mod->m_options)
        g_option_group_add_entries(grp, mod->m_options);
      g_option_context_add_group(context, grp);
    }
  clist_iter_done(iter);

  if (!g_option_context_parse(context, argc, argv, &error))
    {
      log_error("Option parsing failed",
                msg_tag_str("message", error->message),
                msg_tag_str("domain", g_quark_to_string(error->domain)),
                msg_tag_int("code", error->code), NULL);
      g_error_free(error);
      return FALSE;
    }
  return TRUE;
}

static ReportModule *
_tinu_test_find_module(const gchar *name)
{
  CListIterator *iter;
  ReportModule *mod;

  for (iter = clist_iter_new(g_report_modules); clist_iter_next(iter); )
    {
      mod = (ReportModule *)clist_iter_data(iter);

      if (!strcmp(mod->m_name, name))
        {
          clist_iter_done(iter);
          return mod;
        }
    }

  clist_iter_done(iter);
  return NULL;
}

int
tinu_main(int *argc, char **argv[])
{
  FILE *log = NULL;
  gpointer handle = NULL;
  gboolean res;
  gchar *basename = g_path_get_basename(**argv);
  ReportModule *report = NULL;
  TestStatistics *stat = NULL;
  gpointer init_watch = log_register_message_handler(msg_stderr_handler, LOG_ERR, LOGMSG_PROPAGATE);

  g_runtime_name = (*argv)[0];

  signal(SIGSEGV, _tinu_signal_handler);

  tinu_report_add(&g_report_print_module);
  tinu_report_add(&g_report_progam_module);
  tinu_report_add(&g_report_file_module);

  if (!_tinu_options(argc, argv))
    return 1;

  if (g_opt_version)
    _tinu_version();

  if (g_opt_report && NULL == (report = _tinu_test_find_module(g_opt_report)))
    {
      log_error("Could not find given report module",
                msg_tag_str("name", g_opt_report), NULL);
      return 1;
    }

  if (report && report->m_check && !report->m_check(g_opt_stat_verb, g_opt_fancy))
    return 1;

  if (!g_opt_silent)
    {
      if (g_opt_fancy)
        log_register_message_handler(msg_stderr_fancy_handler, g_opt_priority, LOGMSG_PROPAGATE);
      else
        log_register_message_handler(msg_stderr_handler, g_opt_priority, LOGMSG_PROPAGATE);
    }
  atexit(log_clear);
  log_unregister_message_handler(init_watch);

  if (g_opt_syslog)
    {
      openlog(basename, LOG_PID, LOG_USER);
      log_register_message_handler(msg_syslog_handler, g_opt_priority, LOGMSG_PROPAGATE);
    }

  if (g_opt_file)
    {
      log = fopen(g_opt_file, "a");

      if (log)
        handle = log_register_message_handler(msg_file_handler, g_opt_priority, (gpointer)log);
      else
        log_error("Cannot open logfile", msg_tag_str("file", g_opt_file), NULL);
    }

  g_main_test_context.m_sighandle = g_opt_sighandle;
  g_main_test_context.m_leakwatch = g_opt_leakwatch;
#ifdef COREDUMPER_ENABLED
  g_main_test_context.m_core_dir = g_opt_core_dir;
#endif
  if (report && g_opt_stat_verb > STAT_VERB_NONE)
    {
      stat = stat_new(&g_main_test_context);
      stat_start(stat);
    }

  log_debug("Running tests", NULL);
  if (g_opt_suite)
    res = tinu_test_suite_run(&g_main_test_context, g_opt_suite);
  else
    res = tinu_test_all_run(&g_main_test_context);

  if (report)
    {
      stat_stop(stat);
      report->m_handle(stat, g_opt_stat_verb, g_opt_fancy);

      stat_destroy(stat);
    }

  test_context_destroy(&g_main_test_context);
  g_free(basename);

  if (log)
    {
      log_unregister_message_handler(handle);
      fclose(log);
    }

  clist_destroy(g_report_modules, NULL);
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

void
tinu_report_add(const ReportModule *module)
{
  g_report_modules = clist_append(g_report_modules, (gpointer)module);
}
