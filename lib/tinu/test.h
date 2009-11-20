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
