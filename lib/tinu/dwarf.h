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

#ifndef _TINU_DWARF_H
#define _TINU_DWARF_H

#include <glib.h>

#include <features.h>

__BEGIN_DECLS

typedef struct _DwarfEntry
{
  GQuark        m_source;
  gpointer      m_pointer;
  gint          m_lineno;
} DwarfEntry;

typedef struct _DwarfCompUnit
{
  GSList       *m_entries;
  gpointer      m_lowpc;
  gpointer      m_highpc;
} DwarfCompUnit;

typedef struct _DwarfHandle
{
  gchar        *m_filename;
  GSList       *m_entries;
} DwarfHandle;

DwarfHandle *dw_new(const gchar *name);
void dw_destroy(DwarfHandle *self);

const DwarfEntry *dw_lookup(DwarfHandle *self, gpointer ptr, guint32 tolerance);

__END_DECLS

#endif
