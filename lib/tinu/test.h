#ifndef _TINU_TEST_H
#define _TINU_TEST_H

#if 0
#include "../vim.h"
#endif

#include <glib/gtypes.h>
#include <glib/garray.h>

typedef struct _TestCase TestCase;
typedef struct _TestSuite TestSuite;
typedef struct _TestContext TestContext;

typedef gboolean (*TestSetup)(gpointer *);
typedef void (*TestCleanup)(gpointer);
typedef gboolean (*TestFunction)(gpointer);

struct _TestCase
{
  TestSuite      *m_suite;

  const gchar    *m_name;

  TestSetup       m_setup;
  TestCleanup     m_cleanup;
  TestFunction    m_test;
};

struct _TestSuite
{
  const gchar    *m_name;

  GPtrArray      *m_tests;
};

typedef struct _TestContextFuncs
{
  gboolean      (*m_prepare_suite)(TestContext *, TestSuite *);
  void          (*m_done_suite)(TestContext *, TestSuite *, gboolean);
  gboolean      (*m_prepare_test)(TestContext *, TestCase *);
  void          (*m_done_test)(TestContext *, TestCase *, gboolean);
} TestContextFuncs;

struct _TestContext
{
  gboolean      (*m_prepare_suite)(TestContext *, TestSuite *);
  void          (*m_done_suite)(TestContext *, TestSuite *, gboolean);
  gboolean      (*m_prepare_test)(TestContext *, TestCase *);
  void          (*m_done_test)(TestContext *, TestCase *, gboolean);

  GPtrArray      *m_suites;
};

void test_context_init(TestContext *self, const TestContextFuncs *funcs);
void test_context_destroy(TestContext *self);

void test_add(TestContext *self,
              const gchar *suite_name,
              const gchar *test_name,
              TestSetup setup,
              TestCleanup cleanup,
              TestFunction func);

gboolean tinu_test_all_run(TestContext *self);

#endif
