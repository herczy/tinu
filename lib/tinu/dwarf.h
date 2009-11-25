#ifndef _TINU_DWARF_H
#define _TINU_DWARF_H

#include <glib/gtree.h>
#include <glib/gslist.h>
#include <glib/gquark.h>

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

#endif
