/* TINU - Unittesting framework
*
* Copyright (c) 2012, Viktor Hercinger <hercinger.viktor@gmail.com>
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

/** @file meta.h
 * @brief Meta information tools.
 *
 * Meta information is used to automatically collect test case information
 * from the C source files. If a function is created with the specific macros,
 * the function will be collected and added to a special structure.
 */
#ifndef _TINU_META_H
#define _TINU_META_H

#include <tinu/test.h>

__BEGIN_DECLS

/** @brief Metainfo structure
 * This structure is created by the generator tool to create an
 * automatic main() function (or alternatively to just initialize
 * the tinu test list).
 */
typedef struct _TinuMetaInfo
{
  /** Name of the test suite. */
  const gchar  *m_suite;
  /** Name of the test case. */
  const gchar  *m_test;

  /** Test case setup function. */
  TestSetup     m_setup;
  /** Test case setup function. */
  TestCleanup   m_cleanup;
  /** Test case function. */
  TestFunction  m_case;
} TinuMetaInfo;

/** @brief Create a test case function.
 * @param suite Name of the test suite
 * @param testcase Name of the test case
 *
 * This macro creates a function definition header of a test case, but is also
 * used by some external tools to create the metainfo file.
 *
 * The created test function has two arguments:
 *  * test_case - pointer to the current TestCase.
 *  * context - a gpointer user data value, the result of the setup
 *              function, or NULL if no setup was given.
 *
 * Usage:
 *    TEST_FUNCTION(suiteName, testCase)
 *    {
 *      // Assertions
 *    }
 *
 * @see TestFunction
 * @see MARK_TEST_SETUP
 * @see MARK_TEST_CLEANUP
 */
#define TEST_FUNCTION(suite, testcase) \
  void test_function_ ## suite ## _ ## testcase (TestCase *test_case, gpointer context)

/** @brief Create a test setup function for a test case.
 * @param suite Name of the test suite
 * @param testcase Name of the test case
 *
 * This macro creates the setup function definition header of a test case.
 * For more information, see MARK_TEST_FUNCTION.
 *
 * The created test function has two arguments:
 *  * test_case - pointer to the current TestCase.
 *
 * Usage:
 *    TEST_SETUP(suiteName, testCase)
 *    {
 *      // ...
 *      return context;
 *    }
 *
 * @see TestSetup
 * @see MARK_TEST_FUNCTION
 * @see MARK_TEST_CLEANUP
 */
#define TEST_SETUP(suite, testcase) \
  gpointer test_setup_ ## suite ## _ ## testcase (TestCase *test_case)

/** @brief Create a test cleanup function for a test case.
 * @param suite Name of the test suite
 * @param testcase Name of the test case
 *
 * This macro creates the cleanup function definition header of a test case.
 * For more information, see MARK_TEST_FUNCTION.
 *
 * The created test function has two arguments:
 *  * test_case - pointer to the current TestCase.
 *  * context - a gpointer user data value, the result of the setup
 *              function, or NULL if no setup was given.
 *
 * Usage:
 *    TEST_CLEANUP(suiteName, testCase)
 *    {
 *      // Clean up context
 *    }
 *
 * @see TestCleanup
 * @see MARK_TEST_FUNCTION
 * @see MARK_TEST_SETUP
 */
#define TEST_CLEANUP(suite, testcase) \
  void test_cleanup_ ## suite ## _ ## testcase (TestCase *test_case, gpointer context)

/** @brief Add the tests described in the metainfo to the test list.
 * @param metainfo Metainfo array. Must be terminated by an item where the
 * suite and name are NULL.
 */
void tinu_use_metainfo(const TinuMetaInfo *metainfo);

/** @brief Define a main function.
 * @param info Metainfo structure
 *
 * The main function uses the generated meta info structure.
 */
#define DEFINE_MAIN_FUNCTION(info) \
  int main(int argc, char *argv[]) \
  { \
    tinu_use_metainfo(info); \
    return tinu_main(&argc, &argv); \
  }

__END_DECLS

#endif // _TINU_META_H
