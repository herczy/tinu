#ifndef _TINU_MAIN_H
#define _TINU_MAIN_H

#include <tinu/test.h>

int tinu_main(int *argc, char **argv[]);

void tinu_test_add(const gchar *suite_name,
                   const gchar *test_name,
                   TestSetup setup,
                   TestCleanup cleanup,
                   TestFunction func);

#endif
