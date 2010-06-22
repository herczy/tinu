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

class TestSuccess : public CxxTest
{
public:
  void test()
  {
    TINU_ASSERT_TRUE(true);
    TINU_ASSERT_FALSE(false);
  }

};

class TestFailAssert : public CxxTest
{
public:
  void test()
  {
    TINU_ASSERT_TRUE(0);
  }
};

class TestFailSegv : public CxxTest
{
public:
  void test()
  {
    *(char *)NULL = 0;
  }
};

class TestLeakGlobal : public CxxTest
{
public:
  void test()
  {
    malloc(4096);
  }
};

class TestLeakCustom : public CxxTest
{
public:
  void test()
  {
    TestLeakWatch lw;

    malloc(4096);
    free(realloc(malloc(1024), 4096));
  }
};

class TestException : public CxxTest
{
public:
  void test()
  {
    throw Exception("Something went wrong!");
  }
};

int
main(int argc, char *argv[])
{
  CxxTinu tinu(&argc, &argv);

  tinu.add_test<CxxTest>("success", "default");
  tinu.add_test<TestSuccess>("success", "assert");
  tinu.add_test<TestFailAssert>("fail", "assert");
  tinu.add_test<TestFailSegv>("fail", "segfault");
  tinu.add_test<TestLeakGlobal>("leak", "global");
  tinu.add_test<TestLeakCustom>("leak", "custom");
  tinu.add_test<TestException>("fail", "exception");
}
