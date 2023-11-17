#ifndef __COMMANDS_H__
#define __COMMANDS_H__

const char *SOCK_PATH = "/tmp/ploxd";

struct PloxProgCommand {
  char args[8][256];
  int NumArgs;
};

#endif
