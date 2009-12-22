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

#ifndef _TINU_UTILS_H
#define _TINU_UTILS_H

#include <glib/gtypes.h>
#include <glib/garray.h>

#include <tinu/log.h>

#define t_new(type, count) \
  (type *)__allocate(sizeof(type) * (count))

#define t_newp(bytes) \
  __allocate(bytes)

#define t_realloc(ptr, type, count) \
  ((ptr) = (type *)__reallocate(ptr, sizeof(type) * (count)))

#define t_reallocp(ptr, bytes) \
  ((ptr) = __reallocate((ptr), (bytes)))

#define t_free(ptr) __free(ptr)

gpointer __allocate(gsize size);
gpointer __reallocate(gpointer ptr, gsize size);
void     __free(gpointer ptr);

#define _strdup(str) ((str) ? strdup(str) : NULL)
gchar *strndup(const gchar *str, gsize n);

gchar *core_file_name(const gchar *dir, const gchar *suite, const gchar *test);

#define t_assert(cond)                                                  \
  if (!(cond))                                                          \
    {                                                                   \
      log_crit("TINU internal assertion failed",                        \
               msg_tag_str("condition", # cond),                        \
               msg_tag_str("function", __PRETTY_FUNCTION__),            \
               msg_tag_str("filename", __FILE__),                       \
               msg_tag_int("line", __LINE__),                           \
               msg_tag_trace_current("trace", 3), NULL);                \
      exit(1);                                                          \
    }

#define t_raw_assert(cond)                                              \
  if (!(cond))                                                          \
    {                                                                   \
      fprintf(stderr, "Raw assert `" # cond "` failed (%s:%d (%s))\n",  \
              __FILE__, __LINE__, __PRETTY_FUNCTION__);                 \
      exit(1);                                                          \
    }

#endif
