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

#ifndef _TINU_REPORTING_H
#define _TINU_REPORTING_H

#include <glib.h>

#include <tinu/statistics.h>
#include <tinu/names.h>

__BEGIN_DECLS

typedef enum
{
  STAT_VERB_NONE = 0,
  STAT_VERB_SUMMARY,
  STAT_VERB_SUITES,
  STAT_VERB_FULL,
  STAT_VERB_VERBOSE,
} StatisticsVerbosity;

/** Reporting callback to handle statistics after tests */
typedef void (*ReportingHandleCb)(TestStatistics *stat, StatisticsVerbosity verbosity, gboolean enable_colour);

/** Reporting callback to check whether the module is ready */
typedef gboolean (*ReportingCheckCb)(StatisticsVerbosity verbosity, gboolean enable_colour);

/* Report module structure */
typedef struct _ReportModule
{
  const gchar        *m_name;
  const GOptionEntry *m_options;

  ReportingCheckCb    m_check;
  ReportingHandleCb   m_handle;
} ReportModule;

extern const NameTable StatisticsVerbosity_names[];

__END_DECLS

#endif
