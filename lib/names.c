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

#include <glib/gmem.h>

#include <tinu/names.h>

const gchar *
tinu_lookup_key(const NameTable *table, NameTableKey key, gssize *length)
{
  gint i;

  for (i = 0; table[i].m_name; i++)
    {
      if (table[i].m_key == key)
        break;
    }

  if (!table[i].m_name)
    return -1;

  if (length)
    {
      if (table[i].m_length >= 0)
        *length = table[i].m_length;
      else
        *length = strlen(table[i].m_name);
    }

  return table[i].m_name;
}

NameTableKey
tinu_lookup_name(const NameTable *table, const gchar *name, gssize length, gint errkey)
{
  gint i;
  gsize rlen = (length >= 0 ? length : strlen(name));

  for (i = 0; table[i].m_name; i++)
    {
      if (table[i].m_length >= 0 && table[i].m_length != rlen)
        continue;

      if (!strncmp(table[i].m_name, name, rlen))
        break;
    }

  if (!table[i].m_name)
    return errkey;

  return table[i].m_key;
}
