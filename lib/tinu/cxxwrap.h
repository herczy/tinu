/* TINU - Unittesting framework
*
* Copyright (c) 2009-2010, Viktor Hercinger <hercinger.viktor@gmail.com>
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

/** @file cppwrap.h
 * @brief C++ wrapper for TINU
 *
 * This file contains the C++ API for TINU
 */
#ifndef _TINU_CXXWRAP_H
#define _TINU_CXXWRAP_H

#include <sstream>
#include <exception>
#include <string>
#include <map>

#include <tinu/backtrace.h>
#include <tinu/leakwatch.h>
#include <tinu/log.h>
#include <tinu/main.h>

namespace Tinu
{

/* Backtrace */

class CxxBacktrace
{
public:
  class Entry
  {
  public:
    Entry(BacktraceEntry *entry);
    ~Entry();

    gchar *m_function;
    const gchar *m_source;
    guint32 m_line;

  private:
    BacktraceEntry *m_obj;
  };

public:
  CxxBacktrace(guint32 skip = 0, guint32 depth = 0);
  CxxBacktrace(const CxxBacktrace &other);
  CxxBacktrace(Backtrace *trace);
  ~CxxBacktrace();

  inline guint32 depth() const
  { return backtrace_depth(m_obj); }

  CxxBacktrace::Entry at(unsigned int i) const;

  inline const Backtrace *obj() const
  { return m_obj; }

  inline void dump_log(const std::string &prefix, gint priority) const
  { backtrace_dump_log(m_obj, prefix.c_str(), priority); }

private:
  Backtrace *m_obj;
};

/* Exception */

class Exception : public std::exception
{
public:
  inline Exception(const std::string &msg) throw()
    : m_message(msg)
  {}

  virtual ~Exception() throw()
  {}

  virtual inline const char *what() const throw()
  { return m_message.c_str(); }

  virtual inline const CxxBacktrace &trace() const
  { return m_trace; }

  inline void dump_log(const std::string &message,
                       gint exception_priority, gint trace_priority = -1) const
  {
    log_format(exception_priority, message.c_str(), msg_tag_str("exception", what()), NULL);
    m_trace.dump_log("    ", trace_priority < 0 ? exception_priority : trace_priority);
  }

private:
  std::string m_message;
  CxxBacktrace m_trace;
};

/* Leakwatcher */

class CxxLeakwatch
{
  friend void cxx_leakwatch_callback(LeakwatchOperation operation,
                                     gpointer oldptr, gpointer ptr, gsize size,
                                     Backtrace *trace,
                                     gpointer user_data);

public:
  CxxLeakwatch();
  virtual ~CxxLeakwatch();

protected:
  virtual void notify_alloc(gpointer result, gsize size, const CxxBacktrace &trace) = 0;
  virtual void notify_realloc(gpointer source, gpointer result, gsize size, const CxxBacktrace &trace) = 0;
  virtual void notify_free(gpointer pointer, const CxxBacktrace &trace) = 0;

private:
  gpointer m_handle;
};

/* Message */

class CxxMessage
{
public:
  typedef std::map< const gchar *, const gchar * > TagListWrapper;

public:
  inline CxxMessage(Message *msg)
    : m_obj(msg)
  { wrap_tags(); }

  inline ~CxxMessage()
  {}

  void add_tag_raw(MessageTag *tag);
  void remove_tag(const gchar *tag);

  template < class T_tag_value >
  void add_tag(const gchar *key, const T_tag_value &value)
  {
    std::stringstream str;
    str << value;

    add_tag_raw(msg_tag_str(key, str.str().c_str()));
  }

  const gchar *operator[](const gchar *tag) const;
  const TagListWrapper &tags() const;

private:
  void wrap_tags();

private:
  Message *m_obj;
  TagListWrapper m_tag_wrapper;
};

/* MessageHandler wrapping */

class CxxMessageHandler
{
  friend gboolean cxx_log_callback(Message *msg, gpointer user_data);

public:
  CxxMessageHandler(gint max_priority);
  ~CxxMessageHandler();

protected:
  virtual bool notify(const CxxMessage &msg) = 0;

private:
  gpointer m_handle;
};

/* Tinu main */

class CxxTinu
{
public:
  CxxTinu(int *argc, char **argv[]);
  ~CxxTinu();

  template < class T_test >
  void add_test(const gchar *suite, const gchar *name, void (T_test::*test_case)(void))
  {
    Wrapper< T_test > *ud = new Wrapper< T_test >(test_case);
    tinu_test_add_extended(suite,                                   /* Suite name */
                           name,                                    /* Case name */
                           test_setup_wrapper<T_test>,              /* Extended setup */
                           test_cleanup_wrapper<T_test>,            /* Extended cleanup */
                           test_wrapper<T_test>,                    /* Extended test case */
                           reinterpret_cast< gpointer >(ud),        /* Case wrapper */
                           &Wrapper< T_test >::destroy);            /* User data */
  }

  template < class T_test >
  void add_test(const gchar *suite, const gchar *name)
  {
    add_test< T_test >(suite, name, &T_test::test);
  }

private:
  template < class T_test >
  static gpointer test_setup_wrapper(TestCase *test)
  {
    return (gpointer)new T_test;
  }

  template < class T_test >
  static void test_wrapper(TestCase *test, gpointer context)
  {
    typedef void (T_test::*TestCase)(void);
    T_test *testobj = reinterpret_cast< T_test * >(context);
    Wrapper< T_test > *ud = reinterpret_cast< Wrapper < T_test > * >(test->m_user_data);
    try
      {
        ud->execute(testobj);
      }
    catch (Exception &e)
      {
        e.dump_log("TINU exception caught during test", LOG_ERR);
      }
    catch (std::exception &e)
      {
        log_error("Standard exception caught", msg_tag_str("exception", e.what()), NULL);
      }
    catch (...)
      {
        log_error("Unknown exception caught", NULL);
      }
  }

  template < class T_test >
  static void test_cleanup_wrapper(TestCase *test, gpointer context)
  {
    T_test *testobj = reinterpret_cast< T_test * >(context);
    delete testobj;
  }

private:
  template < class T_test >
  class Wrapper
  {
  public:
    typedef void (T_test::*Case)(void);

  public:
    Wrapper(Case test)
      : m_case(test)
    {
    }

    ~Wrapper()
    {
    }

    void execute(T_test *obj)
    {
      (obj->*m_case)();
    }

    static void destroy(gpointer user_data)
    {
      Wrapper< T_test > *obj = reinterpret_cast< Wrapper< T_test > * >(user_data);
      delete obj;
    }

  public:
    Case          m_case;
  };

private:
  int *m_argc;
  char ***m_argv;
};

};

#endif
