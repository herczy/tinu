#include <tinu.h>

using namespace Tinu;

class TestLeakWatch : public CxxLeakwatch
{
public:
  TestLeakWatch()
  {}
  virtual ~TestLeakWatch()
  {}

protected:
  virtual void notify_alloc(gpointer result, gsize size, const CxxBacktrace &trace)
  {
    log_info("Memory allocation event",
             msg_tag_ptr("result", result),
             msg_tag_int("size", size),
             msg_tag_trace("trace", trace.obj()), NULL);
  }

  void notify_realloc(gpointer source, gpointer result, gsize size, const CxxBacktrace &trace)
  {
    log_info("Memory reallocation event",
             msg_tag_ptr("source", source),
             msg_tag_ptr("result", result),
             msg_tag_int("size", size),
             msg_tag_trace("trace", trace.obj()), NULL);
  }

  void notify_free(gpointer pointer, const CxxBacktrace &trace)
  {
    log_info("Memory free event",
             msg_tag_ptr("pointer", pointer),
             msg_tag_trace("trace", trace.obj()), NULL);
  }
};

class TestSuccess
{
public:
  void test_default()
  {
  }

  void test_assert()
  {
    TINU_ASSERT_TRUE(true);
    TINU_ASSERT_FALSE(false);
  }
};

class TestFail
{
public:
  void test_assert()
  {
    TINU_ASSERT_TRUE(0);
  }
  
  void test_segv()
  {
    *(char *)NULL = 0;
  }

  void test_exception()
  {
    throw Exception("Something went wrong!");
  }

};

class TestLeak
{
public:
  void test_global()
  {
    malloc(4096);
  }

  void test_custom()
  {
    TestLeakWatch lw;

    malloc(4096);
    free(realloc(malloc(1024), 4096));
  }
};

int
main(int argc, char *argv[])
{
  CxxTinu tinu(&argc, &argv);

  tinu.add_test("success", "default", &TestSuccess::test_default);
  tinu.add_test("success", "assert", &TestSuccess::test_assert);
  tinu.add_test("fail", "assert", &TestFail::test_assert);
  tinu.add_test("fail", "segfault", &TestFail::test_segv);
  tinu.add_test("leak", "global", &TestLeak::test_global);
  tinu.add_test("leak", "custom", &TestLeak::test_custom);
  tinu.add_test("fail", "exception", &TestFail::test_exception);
}
