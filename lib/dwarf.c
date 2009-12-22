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

#define _LARGEFILE64_SOURCE
#include <libelf.h>
#include <libdwarf.h>

#include <glib/gstrfuncs.h>

#include <unistd.h>
#include <fcntl.h>

#include <tinu/dwarf.h>
#include <tinu/log.h>

#define DWARF_ADDR_TO_GPOINTER(addr) ((gpointer)((int)(addr)))

static inline void
_dw_dwarf_error(Dwarf_Error error, Dwarf_Ptr user_data G_GNUC_UNUSED)
{
  log_error("DWARF error found", msg_tag_str("error", dwarf_errmsg(error)), NULL);
}

static void
_dw_clean_unit(gpointer entry, gpointer data G_GNUC_UNUSED)
{
  g_free(entry);
}

static void
_dw_clean_handle(gpointer unit, gpointer data G_GNUC_UNUSED)
{
  DwarfCompUnit *udata = (DwarfCompUnit *)unit;

  g_slist_foreach(udata->m_entries, _dw_clean_unit, NULL);
  g_slist_free(udata->m_entries);
  g_free(unit);
}

static inline gboolean
_dw_process_unit(DwarfCompUnit **res_unit, Dwarf_Debug dbg, Dwarf_Die die, Dwarf_Error *error)
{
  gboolean result = FALSE;
  int ret;
  gint i;

  Dwarf_Signed lines;
  Dwarf_Line *linebuf = NULL;

  DwarfCompUnit *unit;
  Dwarf_Addr low, high;

  DwarfEntry *entry;
  gchar *filename;
  Dwarf_Addr lineaddr;
  Dwarf_Unsigned lineno;
  GSList *last_entry;

  ret = dwarf_srclines(die, &linebuf, &lines, error);
  *res_unit = NULL;

  if (ret == DW_DLV_NO_ENTRY)
    return TRUE;

  if (ret == DW_DLV_ERROR)
    goto exit;

  ret = dwarf_lowpc(die, &low, error);
  if (ret == DW_DLV_ERROR)
    goto exit;
  else if (ret == DW_DLV_NO_ENTRY)
    {
      result = TRUE;
      goto exit;
    }

  ret = dwarf_highpc(die, &high, error);
  if (ret == DW_DLV_ERROR)
    goto exit;
  else if (ret == DW_DLV_NO_ENTRY)
    {
      result = TRUE;
      goto exit;
    }

  unit = g_new0(DwarfCompUnit, 1);
  unit->m_entries = NULL;
  unit->m_lowpc = DWARF_ADDR_TO_GPOINTER(low);
  unit->m_highpc = DWARF_ADDR_TO_GPOINTER(high);

  for (i = 0, last_entry = NULL; i < lines; i++)
    {
      if (dwarf_linesrc(linebuf[i], &filename, error) != DW_DLV_OK)
        goto exit;

      if (dwarf_lineaddr(linebuf[i], &lineaddr, error) != DW_DLV_OK)
        goto exit;

      if (dwarf_lineno(linebuf[i], &lineno, error) != DW_DLV_OK)
        goto exit;

      entry = g_new0(DwarfEntry, 1);

      entry->m_source = g_quark_from_string(filename);
      entry->m_pointer = DWARF_ADDR_TO_GPOINTER(lineaddr);
      entry->m_lineno = (gint)lineno;

      if (G_LIKELY(last_entry))
        {
          last_entry->next = g_slist_append(NULL, entry);
          last_entry = last_entry->next;
        }
      else
        {
          unit->m_entries = g_slist_append(NULL, entry);
          last_entry = unit->m_entries;
        }
    }

  *res_unit = unit;
  result = TRUE;

exit:
  dwarf_srclines_dealloc(dbg, linebuf, lines);
  dwarf_dealloc(dbg, die, DW_DLA_DIE);

  return result;
}

DwarfHandle *
dw_new(const gchar *name)
{
  DwarfHandle *res = g_new0(DwarfHandle, 1);
  int ret;
  Dwarf_Error error = NULL;

  int fd;
  Dwarf_Debug dbg = NULL;
  Dwarf_Die die = NULL;
  Dwarf_Unsigned next_cu_header = 0;

  DwarfCompUnit *unit;

  GSList *last_unit = NULL;

  res->m_filename = g_strdup(name);
  res->m_entries = NULL;

  log_info("Loading trace info", msg_tag_str("filename", name), NULL);

  fd = open(name, O_RDONLY);
  if (fd == -1)
    {
      log_error("Cannot open runtime file", msg_tag_str("filename", name), NULL);
      goto error;
    }

  if (dwarf_init(fd, DW_DLC_READ, _dw_dwarf_error, NULL, &dbg, &error)
        != DW_DLV_OK)
    {
      if (error)
        goto error;

      log_warn("File does not contain entries", NULL);
      close(fd);
      dwarf_finish(dbg, NULL);
      return res;
    }

  while (DW_DLV_OK == (ret = dwarf_next_cu_header(dbg, NULL, NULL, NULL, NULL,
                                                  &next_cu_header, &error)))
    {
      if (dwarf_siblingof(dbg, NULL, &die, &error) != DW_DLV_OK)
        goto error;

      if (!_dw_process_unit(&unit, dbg, die, &error))
        goto error;

      if (!unit)
        continue;

      if (G_LIKELY(last_unit))
        {
          last_unit->next = g_slist_append(NULL, unit);
          last_unit = last_unit->next;
        }
      else
        {
          res->m_entries = g_slist_append(NULL, unit);
          last_unit = res->m_entries;
        }
    }

  if (ret == DW_DLV_ERROR)
    goto error;

  log_info("DWARF init successfull", NULL);

  close(fd);
  dwarf_finish(dbg, NULL);
  return res;

error:
  close(fd);
  if (error)
    {
      log_error("DWARF error", msg_tag_str("error", dwarf_errmsg(error)), NULL);
      dwarf_dealloc(NULL, error, DW_DLA_ERROR);
    }
  if (dbg)
    dwarf_finish(dbg, NULL);

  dw_destroy(res);
  return NULL;
}

void
dw_destroy(DwarfHandle *self)
{
  g_slist_foreach(self->m_entries, _dw_clean_handle, NULL);
  g_free(self->m_filename);
  g_free(self);
}

const DwarfEntry *
dw_lookup(DwarfHandle *self, gpointer ptr, guint32 tolerance)
{
  GSList *act;

  DwarfCompUnit *unit;
  DwarfEntry *entry;

  /* Find compilation unit */
  for (act = self->m_entries; act; act = act->next)
    {
      unit = (DwarfCompUnit *)act->data;
      if ((ptr >= unit->m_lowpc) && (ptr <= unit->m_highpc))
        break;
    }

  if (!act)
    return NULL;

  /* Find the nearest line number */
  for (act = unit->m_entries; act; act = act->next)
    {
      entry = (DwarfEntry *)act->data;
      if (ptr < entry->m_pointer)
        break;
    }

  return entry;
}
