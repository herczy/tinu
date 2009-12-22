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

#ifndef _TINU_TEST_H
#define _TINU_TEST_H

#include <stdlib.h>

#include <glib/gtypes.h>
#include <glib/garray.h>

#include <tinu/log.h>

typedef struct _TestCase TestCase;
typedef struct _TestSuite TestSuite;
typedef struct _TestContext TestContext;

typedef gboolean (*TestSetup)(gpointer *);
typedef void (*TestCleanup)(gpointer);
typedef void (*TestFunction)(gpointer);

typedef enum
{
  TEST_PASSED = 0,
  TEST_FAILED,
  TEST_SEGFAULT,
  TEST_INTERNAL,
} TestCaseResult;

struct _TestCase
{
  TestSuite      *m_suite;

  const gchar    *m_name;

  TestSetup       m_setup;
  TestCleanup     m_cleanup;
  TestFunction    m_test;

  TestCaseResult  m_result;
};

struct _TestSuite
{
  const gchar    *m_name;

  GPtrArray      *m_tests;

  gboolean        m_passed;
};

typedef struct _TestStatistics
{
  guint32       m_messages[LOG_DEBUG + 1];
  guint32       m_sigsegv;

  guint32       m_passed;
  guint32       m_failed;
} TestStatistics;

struct _TestContext
{
  GPtrArray      *m_suites;

  TestStatistics  m_statistics;

  gboolean        m_sighandle;
  gboolean        m_leakwatch;

  const gchar    *m_core_dir;
};

void test_context_init(TestContext *self);
void test_context_destroy(TestContext *self);

void test_add(TestContext *self,
              const gchar *suite_name,
              const gchar *test_name,
              TestSetup setup,
              TestCleanup cleanup,
              TestFunction func);

gboolean tinu_test_all_run(TestContext *self);
gboolean tinu_test_suite_run(TestContext *self, const gchar *suite_name);

#define TINU_ASSERT_LOG_FAIL(cond)                          \
  log_error("Assertion failed",                             \
            msg_tag_str("condition", #cond),                \
            msg_tag_str("file", __FILE__),                  \
            msg_tag_str("function", __PRETTY_FUNCTION__),   \
            msg_tag_int("line", __LINE__), NULL)

#define TINU_ASSERT_LOG_PASS(cond)                          \
  log_debug("Assertion passed",                             \
            msg_tag_str("condition", #cond),                \
            msg_tag_str("file", __FILE__),                  \
            msg_tag_str("function", __PRETTY_FUNCTION__),   \
            msg_tag_int("line", __LINE__), NULL)

#define TINU_ASSERT_TRUE(cond)                              \
  do                                                        \
    {                                                       \
      if (!(cond))                                          \
        {                                                   \
          TINU_ASSERT_LOG_FAIL(cond);                       \
          abort();                                          \
        }                                                   \
      else                                                  \
        {                                                   \
          TINU_ASSERT_LOG_PASS(cond);                       \
        }                                                   \
    } while (0)

#define TINU_ASSERT_FALSE(cond) TINU_ASSERT_TRUE(!(cond))

#endif
