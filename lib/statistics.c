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

#include <glib.h>

#include <sys/times.h>

static clock_t
_stat_time()
{
  struct tms t;
  return times(&t);
}

static gboolean
_stat_message_counter(Message *msg, gpointer user_data)
{
  TestStatistics *self = (TestStatistics *)user_data;
  self->m_messages[msg->m_priority]++;
  return TRUE;
}

static void
_stat_hook_assert(TestHookID hook_id, TestContext *context, gpointer user_data, va_list vl)
{
  TestStatistics *self = (TestStatistics *)user_data;
  if (va_arg(vl, gboolean))
    {
      self->m_suite_current->m_assertions_passed++;
      self->m_test_current->m_assertions_passed++;
    }
  self->m_suite_current->m_assertions++;
  self->m_test_current->m_assertions++;
}

static void
_stat_hook_case_begin(TestHookID hook_id, TestContext *context, gpointer user_data, va_list vl)
{
  TestStatistics *self = (TestStatistics *)user_data;
  StatTestInfo test_info;

  memset(&test_info, 0, sizeof(test_info));

  test_info.m_test = va_arg(vl, TestCase *);
  test_info.m_start = _stat_time();

  g_array_append_val(self->m_suite_current->m_test_info_list, test_info);
  self->m_test_current = &g_array_index(self->m_suite_current->m_test_info_list, StatTestInfo,
    self->m_suite_current->m_test_info_list->len - 1);
}

static void
_stat_hook_case_end(TestHookID hook_id, TestContext *context, gpointer user_data, va_list vl)
{
  TestStatistics *self = (TestStatistics *)user_data;

  if (self->m_test_current->m_test != va_arg(vl, TestCase *))
    {
      log_error("Duplicate test case in test statistics",
                msg_tag_str("suite", self->m_suite_current->m_suite->m_name),
                msg_tag_str("testcase", self->m_test_current->m_test->m_name), NULL);
      return;
    }
  self->m_test_current->m_end = _stat_time();
  self->m_test_current->m_result = va_arg(vl, TestCaseResult);

  switch (self->m_test_current->m_result)
    {
      case TEST_PASSED :
        self->m_passed++;
        break;

      case TEST_ABORT :
      case TEST_INTERNAL :
      case TEST_FAILED :
        self->m_failed++;
        break;

      case TEST_SEGFAULT :
        self->m_sigsegv++;
        self->m_failed++;
        break;

      default :
        g_assert_not_reached();
    }

  self->m_test_current = NULL;
}

static void
_stat_hook_suite_begin(TestHookID hook_id, TestContext *context, gpointer user_data, va_list vl)
{
  TestStatistics *self = (TestStatistics *)user_data;
  StatSuiteInfo suite_info;

  memset(&suite_info, 0, sizeof(suite_info));

  suite_info.m_suite = va_arg(vl, TestSuite *);
  suite_info.m_start = _stat_time();
  suite_info.m_test_info_list = g_array_new(FALSE, FALSE, sizeof(StatTestInfo));

  g_array_append_val(self->m_suite_info_list, suite_info);
  self->m_suite_current = &g_array_index(self->m_suite_info_list, StatSuiteInfo,
    self->m_suite_info_list->len - 1);
}

static void
_stat_hook_suite_end(TestHookID hook_id, TestContext *context, gpointer user_data, va_list vl)
{
  TestStatistics *self = (TestStatistics *)user_data;

  if (self->m_suite_current->m_suite != va_arg(vl, TestSuite *))
    {
      log_error("Duplicate suite in test statistics",
                msg_tag_str("suite", self->m_suite_current->m_suite->m_name), NULL);
      return;
    }
  self->m_suite_current->m_end = _stat_time();
  self->m_suite_current->m_result = va_arg(vl, gboolean);
  self->m_suite_current = NULL;
}

static void
_stat_hook_leakwatch(TestHookID hook_id, TestContext *context, gpointer user_data, va_list vl)
{
  TestStatistics *self = (TestStatistics *)user_data;

  if (self->m_test_current->m_test != va_arg(vl, TestCase *))
    {
      log_error("Duplicate test case in test statistics",
                msg_tag_str("suite", self->m_suite_current->m_suite->m_name),
                msg_tag_str("testcase", self->m_test_current->m_test->m_name), NULL);
      return;
    }

  self->m_test_current->m_leaked_bytes = va_arg(vl, gsize);
}

static TestHookCb g_stat_hooks[TEST_HOOK_MAX] = {
  [TEST_HOOK_ASSERT]            = &_stat_hook_assert,
  [TEST_HOOK_SIGNAL_ABORT]      = NULL,
  [TEST_HOOK_SIGNAL_SEGFAULT]   = NULL,
  [TEST_HOOK_BEFORE_TEST]       = &_stat_hook_case_begin,
  [TEST_HOOK_AFTER_TEST]        = &_stat_hook_case_end,
  [TEST_HOOK_BEFORE_SUITE]      = &_stat_hook_suite_begin,
  [TEST_HOOK_AFTER_SUITE]       = &_stat_hook_suite_end,
  [TEST_HOOK_LEAKINFO]          = &_stat_hook_leakwatch,
};

TestStatistics *
stat_new(TestContext *context)
{
  TestStatistics *res = g_new0(TestStatistics, 1);
  res->m_context = context;
  res->m_suite_info_list = g_array_new(FALSE, FALSE, sizeof(StatSuiteInfo));
  return res;
}

void
stat_destroy(TestStatistics *self)
{
  StatSuiteInfo *suite;
  gint i;

  stat_stop(self);

  for (i = 0; i < self->m_suite_info_list->len; i++)
    {
      suite = &g_array_index(self->m_suite_info_list, StatSuiteInfo, i);
      g_array_free(suite->m_test_info_list, TRUE);
    }

  g_array_free(self->m_suite_info_list, TRUE);
  g_free(self);
}

void
stat_start(TestStatistics *self)
{
  self->m_log_handle = log_register_message_handler(_stat_message_counter, LOG_DEBUG, self);
  test_register_multiple_hooks(self->m_context, g_stat_hooks, (gpointer)self);
}

void
stat_stop(TestStatistics *self)
{
  test_unregister_multiple_hooks(self->m_context, g_stat_hooks, (gpointer)self);

  if (self->m_log_handle)
    {
      log_unregister_message_handler(self->m_log_handle);
      self->m_log_handle = NULL;
    }
}

