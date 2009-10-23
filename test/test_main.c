#include <stdlib.h>

#include <applog.h>

#include <tinu/main.h>
#include <tinu/utils.h>

gboolean
setup(gpointer *ctx)
{
  *ctx = (gpointer)t_new(int, 1);
  return TRUE;
}

void
cleanup(gpointer ctx)
{
  log_debug("Test done", msg_tag_int("context", *(int *)ctx), NULL);
  t_free(ctx);
}

void
succeed(gpointer ctx)
{
  *(int *)ctx += 5;
}

void
fail(gpointer ctx G_GNUC_UNUSED)
{
  TINU_ASSERT_TRUE(0);
}

void
leak(gpointer ctx G_GNUC_UNUSED)
{
  void *leakme = realloc(malloc(1024), 2);
}

void
segfault(gpointer ctx G_GNUC_UNUSED)
{
  *(gint *)NULL = 1;
}

int
main(int argc, char *argv[])
{
  tinu_test_add("suite", "succeed", NULL, NULL, NULL);
  tinu_test_add("suite", "fail", NULL, NULL, fail);
  tinu_test_add("suite", "full", setup, cleanup, succeed);

  tinu_test_add("leak", "leak", NULL, NULL, leak);

  tinu_test_add("signal", "segv", NULL, NULL, segfault);
  tinu_test_add("signal", "segv2", NULL, NULL, segfault);
  tinu_test_add("signal", "segv3", NULL, NULL, segfault);
  tinu_test_add("signal", "segv4", NULL, NULL, segfault);

  return tinu_main(&argc, &argv);
}
