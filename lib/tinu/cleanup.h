#ifndef _TINU_CLEANUP_H
#define _TINU_CLEANUP_H

#include <glib/gtypes.h>

typedef void (*CleanupFunc)(gpointer);

gpointer tinu_atexit(CleanupFunc cleanup, gpointer user_data);
void tinu_atexit_clear(gpointer handle);

void tinu_reset();

#endif
