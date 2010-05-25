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

#ifndef _TINU_BACKTRACE_H
#define _TINU_BACKTRACE_H

#include <stdint.h>
#include <stdio.h>

#include <tinu/log.h>
#include <tinu/config.h>

__BEGIN_DECLS

typedef struct _BacktraceEntry
{
  gpointer    m_ptr;
  gsize       m_offset;

  gchar      *m_function;
  gchar      *m_file;
} BacktraceEntry;

#define BACKTRACE_ENTRY_INVALID          ((BacktraceEntry *)-1)
typedef void (*DumpCallback)(const BacktraceEntry *entry, gpointer ud);

#define MAX_DEPTH       4096

typedef struct _Backtrace Backtrace;

Backtrace *backtrace_create(guint32 skip);
Backtrace *backtrace_create_depth(guint32 depth, guint32 skip);
Backtrace *backtrace_reference(Backtrace *self);
void backtrace_unreference(Backtrace *self);

guint32 backtrace_depth(const Backtrace *self);

void backtrace_dump_log(const Backtrace *self, const gchar *msg_prefix, gint priority);
void backtrace_dump_file(const Backtrace *self, FILE *output, guint8 indent);
void backtrace_dump(const Backtrace *self, DumpCallback callback, gpointer user_data);

BacktraceEntry *backtrace_line(const Backtrace *self, guint32 index);
gboolean backtrace_resolv_lines(const BacktraceEntry *entry, const gchar **src, guint32 *line);
void backtrace_entry_destroy(BacktraceEntry *self);

MessageTag *msg_tag_trace(const gchar *tag, const Backtrace *trace);
MessageTag *msg_tag_trace_current(const gchar *tag, int skip);

#ifdef ELFDEBUG_ENABLED
extern gchar *(*g_demangler)(const gchar *function);
#endif

__END_DECLS

#endif
