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

#ifndef _TINU_STATISTICS_H
#define _TINU_STATISTICS_H

#include <time.h>

#include <tinu/test.h>

#define HOOK_NOT_REGISTERED ((guint32)-1)

typedef struct _StatTestInfo
{
  /** The test the result belongs to */
  TestCase         *m_test;

  /** Result of the test */
  TestCaseResult    m_result;

  /** Start time of the test */
  clock_t           m_start;
  /** End time of the test */
  clock_t           m_end;

  /** Number of assertions in the test */
  guint             m_assertions;
  /** Number of passed assertions in the test */
  guint             m_assertions_passed;

  /** Bytes leaked during test execution */
  gsize             m_leaked_bytes;
} StatTestInfo;

typedef struct _StatSuiteInfo
{
  /** The suite the result belongs to */
  TestSuite        *m_suite;

  /** Result of the suite */
  gboolean          m_result;

  /** Start time of the test */
  clock_t           m_start;
  /** End time of the test */
  clock_t           m_end;

  /** Number of assertions in the suite */
  guint             m_assertions;
  /** Number of passed assertions in the suite */
  guint             m_assertions_passed;

  /** Test information (result, status, etc.) */
  GArray           *m_test_info_list;
} StatSuiteInfo;

/** @brief Test statistics
 *
 * Contains statistical information about the test cases and
 * emitted messages.
 *
 * @note Do not use directly. It may change at any time.
 */
typedef struct _TestStatistics
{
  /** The test context which the statistics refers to */
  TestContext      *m_context;

  /** Registered log handle */
  gpointer          m_log_handle;

  StatTestInfo     *m_test_current;
  StatSuiteInfo    *m_suite_current;

  /** Number of emitted messages by priority */
  guint32           m_messages[LOG_DEBUG + 1];
  /** Number of segmentation faults */
  guint32           m_sigsegv;

  /** Number of tests passed */
  guint32           m_passed;
  /** Number of tests failed */
  guint32           m_failed;

  /** Suite information (like m_test_info) */
  GArray           *m_suite_info_list;
} TestStatistics;

TestStatistics *stat_new(TestContext *context);
void stat_destroy(TestStatistics *self);

void stat_start(TestStatistics *self);
void stat_stop(TestStatistics *self);

#endif
