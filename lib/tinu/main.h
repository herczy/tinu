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

/** @file main.h
 * @brief The main API of TINU
 *
 * This file contains the main API for TINU. The functions here should be
 * used to add test cases (globally) and to start the test.
 */
#ifndef _TINU_MAIN_H
#define _TINU_MAIN_H

#include <glib.h>

#include <tinu/test.h>
#include <tinu/statistics.h>
#include <tinu/reporting.h>

__BEGIN_DECLS

/** @brief Main function of the framework
 * @param argc Pointer to the argument count (as given by main)
 * @param argv Pointer to the argument list (as given by main)
 * @return If all the suites run succeed return 0, on error return -1
 *
 * This function is the "core" of the framework. This function should be
 * run and the result of it should be the exit code.
 */
int tinu_main(int *argc, char **argv[]);

/** @brief Add a test to the framework
 * @param suite_name Suite test belongs to
 * @param test_name Name of the current test
 * @param setup Setup function (NULL if none)
 * @param cleanup Cleanup function (NULL if none)
 * @param func Test function (NULL if none)
 * @see test_add
 *
 * Similar to test_add but there is no test context required. It adds the
 * tests to the main test context defined in test.c through test_add.
 */
void tinu_test_add(const gchar *suite_name,
                   const gchar *test_name,
                   TestSetupSimple setup,
                   TestCleanupSimple cleanup,
                   TestFunctionSimple func);

/** @brief Add an extended test to the framework
 * @param suite_name Suite test belongs to
 * @param test_name Name of the current test
 * @param setup Setup function (NULL if none)
 * @param cleanup Cleanup function (NULL if none)
 * @param func Test function (NULL if none)
 * @param user_data User data put into TestCase structure
 * @param user_data_cleanup Called to free user data
 * @see test_add
 * @see test_add_extended
 *
 * Similar to test_add_extended but there is no test context required. It adds
 * the tests to the main test context defined in test.c through test_add.
 */
void tinu_test_add_extended(const gchar *suite_name,
                            const gchar *test_name,
                            TestSetup setup,
                            TestCleanup cleanup,
                            TestFunction func,
                            gpointer user_data,
                            CleanupFunction user_data_cleanup);

/** @brief Add a reporting facility to the framework
 * @param module Report module descriptor
 *
 * Registers a reporting facility. This facility can be used to handle the statistics.
 * Each such `module' contains a set of command line options that can be used to extend the
 * reporting functionality
 */
void tinu_report_add(const ReportModule *module);

__END_DECLS

#endif
