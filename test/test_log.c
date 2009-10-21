#include <tinu/log.h>

int
main(int argc, char *argv[])
{
  tinu_register_message_handler(msg_stderr_fancy_handler, LOG_DEBUG, LOGMSG_PROPAGATE);

  log_abort("Abort message", msg_tag_str("tag", "test"), NULL);
  log_error("Error message", msg_tag_str("tag", "test"), NULL);
  log_warn("Warning message", msg_tag_str("tag", "test"), NULL);
  log_notice("Notice message", msg_tag_str("tag", "test"), NULL);
  log_info("Info message", msg_tag_str("tag", "test"), NULL);
  log_debug("Debug message", msg_tag_str("tag", "test"), NULL);

  return 0;
}

