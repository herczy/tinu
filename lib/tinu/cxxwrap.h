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

/* Exception */

class Exception : public std::exception
{
public:
  inline Exception(const std::string &msg) throw()
    : m_message(msg)
  {}

  virtual ~Exception() throw()
  {}

  virtual inline const char *what() throw()
  { return m_message.c_str(); }

private:
  std::string m_message;
};

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

  CxxBacktrace::Entry at(int i) const;

  inline const Backtrace *obj() const
  { return m_obj; }

private:
  Backtrace *m_obj;
};

/* Leakwatcher */

class CxxLeakwatch
{
  friend void cxx_leakwatch_callback(LeakwatchOperation operation,
                                     gpointer oldptr, gpointer ptr, gsize size,
                                     Backtrace *trace,
                                     gpointer user_data);
public:
  typedef enum { MALLOC, REALLOC, FREE } Operation;

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

/* Test case wrapper */

class CxxTest
{
  friend class CxxTinu;

public:
  inline CxxTest()
  {}
  virtual inline ~CxxTest()
  {}

  virtual inline void test()
  {}
};

/* Tinu main */

class CxxTinu
{
public:
  CxxTinu(int *argc, char **argv[]);
  ~CxxTinu();

  template < class T_test >
  void add_test(const gchar *suite, const gchar *name)
  {
    tinu_test_add(suite, name, test_setup_wrapper<T_test>, test_cleanup_wrapper, test_wrapper);
  }

private:
  template < class T_test >
  static gpointer test_setup_wrapper()
  {
    return (gpointer)new T_test;
  }

  static void test_cleanup_wrapper(gpointer context);
  static void test_wrapper(gpointer context);

private:
  int *m_argc;
  char ***m_argv;
};

};

#endif
