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

#include <stdlib.h>
#include <string.h>

#include <sys/mman.h>
#include <unistd.h>

#include <tinu/utils.h>

#include <glib/gmem.h>

struct _SharedMemory
{
  gsize       m_size;
  gsize       m_aligned;

  gchar       m_ptr[0];
};

static inline gsize
_align(gsize size)
{
  gsize pg = getpagesize();
  return (size / pg + 1) * pg;
}

gpointer 
__allocate(gsize size)
{
  gpointer ptr;

  t_raw_assert(size > 0);
  ptr = g_try_malloc0(size);
  t_raw_assert(ptr);
  return ptr;
}

gpointer 
__reallocate(gpointer ptr, gsize size)
{
  gpointer res;
  if (size == 0)
    {
      __free(ptr);
      return NULL;
    }

  res = g_try_realloc(ptr, size);
  t_raw_assert(res);
  return res;
}

void
__free(gpointer ptr)
{
  if (ptr)
    g_free(ptr);
}

gchar *
core_file_name(const gchar *dir, const gchar *suite, const gchar *test)
{
  static gchar res[1024];

  snprintf(res, sizeof(res), "%s/core.%s.%s", dir, suite, test);
  return res;
}
