#include <unistd.h>
#include <stdio.h>

#include<string>
#include<vector>
#include <iostream>

int main(int argc, char *argv[])
{
  char buffer[1024];

  std::vector<std::string> env;
  env.push_back("LD_PRELOAD=/usr/home/ryan/ploxd/build/src/container/libshim.so");

  int ploxd = std::stoi(argv[argc - 1]);

  snprintf(buffer, 1024, "PLOXD_SOCKET=%s", argv[argc - 1]);
  env.push_back(std::string(buffer, strlen(buffer)));

  argv[argc - 1] = nullptr;

  std::vector<char *> envp;
  for (auto &e : env) {
    envp.push_back((char *)e.c_str());
  }

  envp.push_back(nullptr);

  std::cout << argv[0] << std::endl;

  int writeout = write(ploxd, "HELLO", 6);
  if (writeout < 0) {
    perror("NOOO");
    return -1;
  }

  execve(argv[0], argv, envp.data());
  return 0;
}
