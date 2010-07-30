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

/** @file test.h
 * @brief Test cases and their internal representations
 *
 * This file contains the API for the individual test cases.
 */
#ifndef _TINU_TEST_H
#define _TINU_TEST_H

#include <stdlib.h>
#include <string.h>

#include <glib/gtypes.h>
#include <glib/garray.h>

#include <tinu/log.h>
#include <tinu/clist.h>
#include <tinu/names.h>

__BEGIN_DECLS

typedef struct _TestCase TestCase;
typedef struct _TestSuite TestSuite;
typedef struct _TestContext TestContext;

/** @brief Setup function
 * The result is the _context_ of the test, meaning that it will be passed to
 * the test and later cleaned up. The rationale behind this is that the same
 * dataset can be prepared and cleaned up for different tests.
 */
typedef gpointer (*TestSetup)(void);
/** @brief Cleanup function
 * This function is used to clean up the previously prepared context. You should
 * free the allocated memory if neccessary.
 */
typedef void (*TestCleanup)(gpointer);
/** @brief Test function
 * TestFunction is the main body of the test case. Generaly speaking it should be
 * independent of the preparation/cleanup (i.e. it should work independently of the 
 * actual initialization as much as possible).
 */
typedef void (*TestFunction)(gpointer);

/**
 * Signifies the result of a given test case and suite.
 *
 * @note Public because the TEST_HOOK_AFTER_TEST hook uses it as a parameter.
 */
typedef enum
{
  /** The test case hasn't been run yet.
   * @note This is a strictly internal name that should never be the result. */
  TEST_NONE = -1,
  /** The test case passed */
  TEST_PASSED = 0,
  /** The test case failed */
  TEST_FAILED,
  /** The test case was aborted via a SIGABRT */
  TEST_ABORT,
  /** The test case resulted in a segmentation fault */
  TEST_SEGFAULT,
  /** The test case failed because some internal error */
  TEST_INTERNAL,
} TestCaseResult;

typedef enum
{
  /** Hook indicating an assertion was evaluated */
  TEST_HOOK_ASSERT = 0,

  /** Hook indicating a SIGABRT */
  TEST_HOOK_SIGNAL_ABORT,
  /** Hook indicating a SIGSEGV */
  TEST_HOOK_SIGNAL_SEGFAULT,

  /** Hook indicating the beginning of a test execution */
  TEST_HOOK_BEFORE_TEST,
  /** Hook indicating the ending of a test execution */
  TEST_HOOK_AFTER_TEST,
  /** Hook indicating the beginning of a suite execution */
  TEST_HOOK_BEFORE_SUITE,
  /** Hook indicating the ending of a suite execution */
  TEST_HOOK_AFTER_SUITE,

  /** The last hook */
  TEST_HOOK_MAX,

  /** Used by register/unregister to indicate all hooks */
  TEST_HOOK_ALL = 0xFFFF,
} TestHookID;

typedef void (*TestHookCb)(TestHookID hook_id, TestContext *context, gpointer user_data, va_list vl);

/** @brief Individual test case
 *
 * Representation of an individual test case.
 *
 * @note Do not use directly. It may change at any time.
 */
struct _TestCase
{
  /** The suite the case belongs to */
  TestSuite      *m_suite;

  /** Displayed name of the case */
  const gchar    *m_name;

  /** Setup function of the test. Called first. */
  TestSetup       m_setup;
  /** Cleanup function of the test. Called last. */
  TestCleanup     m_cleanup;
  /** Test function. This should be the "body" of the test */
  TestFunction    m_test;
};

/** @brief Test suite
 *
 * Suites are the groups in which test cases are handled. They are supposed
 * to help the grouping of a test. They can be executed separately with
 * --suite <suite name> from the command line.
 *
 * @note Do not use directly. It may change at any time.
 */
struct _TestSuite
{
  /** Displayed name of the test suite */
  const gchar    *m_name;

  /** Contains the individual test cases */
  GPtrArray      *m_tests;
};

/** @brief Hook entry
 *
 * This entry contains a hook callback and a user data to pass along
 *
 * @note Do not use directly. It may change at any time
 */
typedef struct _TestHookEntry
{
  /** Hook callback */
  TestHookCb      m_hook;

  /** Hook callback user data */
  gpointer        m_user_data;
} TestHookEntry;

/** @brief Test context
 *
 * This is the context of a test: it contains all the test suites (which contain the test
 * cases), neccessary information about running the tests, etc.
 *
 * @note Do not use directly. It may change at any time.
 */
struct _TestContext
{
  /** List of suites */
  GPtrArray      *m_suites;

  /** Enables/disables signal handling */
  gboolean        m_sighandle;
  /** Enables/disables built-in leak detection */
  gboolean        m_leakwatch;

  /** Directory to put the cores in (or NULL if no cores
   * should be stored
   */
  const gchar    *m_core_dir;

  /** Test hook callbacks */
  CList          *m_hooks[TEST_HOOK_MAX];
};

/** @brief Get a user-friendly description for a TestCaseResult
 * @param result The test case result
 *
 * Gets a user-friendly name for the test case result enum.
 */
const gchar *test_result_name(TestCaseResult result);

/** @brief Initialize test context
 * @param self The initialized context
 *
 * Initializes the context. Parameters (like m_sighandle or m_sigsegv)
 * should be set manually.
 */
void test_context_init(TestContext *self);
/** @brief Clean up test context
 * @param self Test context
 */
void test_context_destroy(TestContext *self);

/** @brief Add test to context
 * @param self Test context
 * @param suite_name Suite test belongs to
 * @param test_name Name of the current test
 * @param setup Setup function (NULL if none)
 * @param cleanup Cleanup function (NULL if none)
 * @param func Test function (NULL if none)
 *
 * This function is used to fill a test context with test cases. Suites are
 * handled implicitly, so there is no need to add them separately.
 */
void test_add(TestContext *self,
              const gchar *suite_name,
              const gchar *test_name,
              TestSetup setup,
              TestCleanup cleanup,
              TestFunction func);

/** @brief Register a hook in the test context
 * @param self Test context
 * @param hook_id ID of hook to register (TEST_HOOK_ALL to register all hooks)
 * @param hook The hook callback function
 * @param user_data The hook callback user data
 *
 * Registers a hook callback which is called at different points during
 * the test/suite execution.
 */
void test_register_hook(TestContext *self, TestHookID hook_id,
  TestHookCb hook, gpointer user_data);

/** @brief Register multiple hooks with the same user data
 * @param self Test context
 * @param hooks An array of hooks. Size should be TEST_HOOK_MAX.
 *  NULL callbacks will not be registered
 * @param user_data The hook callback user data
 *
 * Registers the hooks in the given list, like test_register_hook, but with multiple
 * callbacks using the same user data. This is only a conveinence function.
 *
 * @note Use this for `hook' modules
 */
void test_register_multiple_hooks(TestContext *self, TestHookCb *hooks, gpointer user_data);

/** @brief Unregister hook from the test context
 * @param self Test context
 * @param hook_id ID of hook to unregister (TEST_HOOK_ALL to unregister all hooks)
 * @param hook The hook callback function to remove (NULL to remove all hooks)
 * @param user_data The hook callback user data to remove (NULL to remove a specific callback)
 *
 * Unregisters a hook from the test context.
 */
void test_unregister_hook(TestContext *self, TestHookID hook_id, TestHookCb hook, gpointer user_data);

/** @brief Unregister multiple hooks with the same user data
 * @param self Test context
 * @param hooks An array of hooks. Size should be TEST_HOOK_MAX.
 *  NULL callbacks will not be unregistered
 * @param user_data The hook callback user data
 *
 * Unregisters the hooks in the given list, like test_unregister_hook, but with multiple
 * callbacks using the same user data. This is only a conveinence function.
 *
 * @note Use this for `hook' modules
 */
void test_unregister_multiple_hooks(TestContext *self, TestHookCb *hooks, gpointer user_data);

/** @brief Run all tests
 * @param self Test context
 * @return Wheter all tests succeeded.
 *
 * Runs all tests and collects the statistics
 */
gboolean tinu_test_all_run(TestContext *self);
/** @brief Run an individual suite
 * @param self Test context
 * @param suite_name Suite to run
 * @return Wheter the test suite succeeded.
 *
 * This runs only a distinct test suite. Suites are accessed by name.
 */
gboolean tinu_test_suite_run(TestContext *self, const gchar *suite_name);

/** @brief Evaluate an assertion
 * @param condition The condition to be asserted.
 * @param assert_type A string that will be set in the message as the assert type
 * (e.g. string compare, assert true, assert false, etc.)
 * @param condstr The condition converted to a string.
 * @param file The file where the assertion comes from
 * @param func The function where the assertion comes from
 * @param line The line where the assertion comes from 
 * @param tag0 First tag to add to any message or NULL if none are appended
 * @param ... Additional tags, NULL-terminated
 * @return Same value as of condition
 *
 * Before this function came into effect all assertions were macros,
 * but this proved to be inflexible when hooking came into the picture.
 *
 * @note Do not use directly! Use the macros. This way the API does not
 * change. This function can change.
 */
gboolean tinu_test_assert(gboolean condition, const gchar *assert_type, const gchar *condstr,
  const gchar *file, const gchar *func, gint line, MessageTag *tag0, ...);

/** @brief `TRUE' (or positive) Assertion
 * @param cond Assertion condition
 *
 * This macro checks the condition given and if fails, the test case
 * will also fail. But this assertion does not emit a SIGABRT signal. 
 */
#define TINU_ASSERT_TRUE(cond)                  \
  tinu_test_assert(((cond) ? TRUE : FALSE),     \
                   "positive",                  \
                   #cond,                       \
                   __FILE__,                    \
                   __PRETTY_FUNCTION__,         \
                   __LINE__,                    \
                   NULL)

/** @brief `FALSE' (or negative) Assertion
 * @param cond Assertion condition
 *
 * This macro checks the condition given and if fails, the test case
 * will also fail. But this assertion does not emit a SIGABRT signal.
 */
#define TINU_ASSERT_FALSE(cond)                 \
  tinu_test_assert(!(cond),                     \
                   "negative",                  \
                   #cond,                       \
                   __FILE__,                    \
                   __PRETTY_FUNCTION__,         \
                   __LINE__,                    \
                   NULL)

/** @brief Check if the two strings are equal
 * @param str1 First string
 * @param str2 Second string
 *
 * This macro checks wheter two strings are equal or not. As with
 * TINU_ASSERT_TRUE and TINU_ASSERT_FALSE this does not emit a
 * SIGABRT signal.
 */
#define TINU_ASSERT_STREQ(str1, str2)           \
  tinu_test_assert(strcmp((str1), (str2)) == 0, \
                   "string equality",           \
                   "str1 == str2",              \
                   __FILE__,                    \
                   __PRETTY_FUNCTION__,         \
                   __LINE__,                    \
                   msg_tag_str("str1", str1),   \
                   msg_tag_str("str2", str2),   \
                   NULL)

/** @brief Make an assertion fatal
 * @param assertion The assertion call, should be one of the above
 *
 * This macro is intended as a wrapper to be used with the previously
 * mentioned macros, like 'FATAL(TINU_ASSERT_TRUE(cond))'. If the
 * assertion fails, the test case will abort. This is usefull if the
 * failure of the assertion means that the test case cleanup function
 * should not be run (e.g. the test context became corrupt).
 */
#define FATAL(assertion) if (!(assertion)) { abort(); }

/** @brief Table containing the test case result IDs
 */
extern const NameTable TestCaseResult_names[];

/** @brief Table containing the test hook IDs
 */
extern const NameTable TestHookID_names[];

__END_DECLS

#endif
