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

#include <string.h>
#include <cxxabi.h>

#include <glib.h>

#include <tinu/utils.h>
#include <tinu/cxxwrap.h>

namespace Tinu
{

gchar *
cxx_demangle(const gchar *function)
{
  gchar tmp[4096];
  gsize size = sizeof(tmp);
  int flags;

  abi::__cxa_demangle(function, tmp, &size, &flags);
  return (flags == 0 ? g_strndup(tmp, size) : g_strdup(function));
}
/* BacktraceEntry */

CxxBacktrace::Entry::Entry(BacktraceEntry *entry)
  : m_obj(entry)
{
  if (!backtrace_resolv_lines(entry, &m_source, &m_line))
    {
      m_source = "(unknown)";
      m_line = 0;
    }
}

CxxBacktrace::Entry::~Entry()
{
  backtrace_entry_destroy(m_obj);
  if (m_function)
    t_free(m_function);
}

/* Backtrace */

CxxBacktrace::CxxBacktrace(guint32 skip, guint32 depth)
  : m_obj(backtrace_create_depth(depth, skip))
{}

CxxBacktrace::CxxBacktrace(const CxxBacktrace &other)
  : m_obj(backtrace_reference(const_cast<Backtrace *>(other.m_obj)))
{}

CxxBacktrace::CxxBacktrace(Backtrace *trace)
  : m_obj(backtrace_reference(trace))
{}

CxxBacktrace::~CxxBacktrace()
{
  backtrace_unreference(m_obj);
}

CxxBacktrace::Entry
CxxBacktrace::at(unsigned int i) const
{
  if (i > depth())
    throw Exception("Index out of range");

  return Entry(backtrace_line(m_obj, i));
}

/* Leakwatch */

void
cxx_leakwatch_callback(LeakwatchOperation operation,
                       gpointer oldptr, gpointer ptr, gsize size,
                       Backtrace *trace,
                       gpointer user_data)
{
  CxxLeakwatch *self = (CxxLeakwatch *)user_data;
  CxxBacktrace cxxtrace(trace);

  switch (operation)
    {
      case LEAKWATCH_OPERATION_MALLOC :
        self->notify_alloc(ptr, size, cxxtrace);
        break;

      case LEAKWATCH_OPERATION_REALLOC :
        self->notify_realloc(oldptr, ptr, size, cxxtrace);
        break;

      case LEAKWATCH_OPERATION_FREE :
        self->notify_free(ptr, cxxtrace);
        break;
    }
}

CxxLeakwatch::CxxLeakwatch()
{
  m_handle = tinu_register_watch(cxx_leakwatch_callback, this);
}

CxxLeakwatch::~CxxLeakwatch()
{
  tinu_unregister_watch(m_handle);
}

/* Message */

const gchar *
CxxMessage::operator[](const gchar *tag) const
{
  return const_cast<TagListWrapper &>(m_tag_wrapper)[tag];
}

const CxxMessage::TagListWrapper &
CxxMessage::tags() const
{
  return m_tag_wrapper;
}

void
CxxMessage::wrap_tags()
{
  m_tag_wrapper.clear();
  for (gint i = 0; i < m_obj->m_tag_count; i++)
    {
      m_tag_wrapper[m_obj->m_tags[i]->m_tag] = m_obj->m_tags[i]->m_value;
    }
}

void
CxxMessage::add_tag_raw(MessageTag *tag)
{
  msg_append(m_obj, tag, NULL);
  m_tag_wrapper[tag->m_tag] = tag->m_value;
}

void
CxxMessage::remove_tag(const gchar *tag)
{
  if (m_tag_wrapper.find(tag) == m_tag_wrapper.end())
    return;

  msg_remove_tag(m_obj, tag);
}

/* MessageHandler */

gboolean
cxx_log_callback(Message *msg, gpointer user_data)
{
  CxxMessage cxxmsg(msg);
  CxxMessageHandler *handler = (CxxMessageHandler *)user_data;
  return (gboolean)handler->notify(cxxmsg);
}

CxxMessageHandler::CxxMessageHandler(gint max_priority)
{
  m_handle = log_register_message_handler(cxx_log_callback, max_priority, this);
}

CxxMessageHandler::~CxxMessageHandler()
{
  log_unregister_message_handler(m_handle);
}

/* Test case wrapper */

CxxTinu::CxxTinu(int *argc, char **argv[])
  : m_argc(argc),
  m_argv(argv)
{
#ifdef ELFDEBUG_ENABLED
  g_demangler = cxx_demangle;
#endif
}

CxxTinu::~CxxTinu()
{
  exit(tinu_main(m_argc, m_argv));
}

}
