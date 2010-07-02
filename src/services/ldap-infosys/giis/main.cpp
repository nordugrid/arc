#include "Server.h"

#include <unistd.h>
#include <signal.h>

#include <cstdio>
#include <iostream>
#include <string>
#include <cstring>
#include <cstdlib>

int main(int argv, char **argc) {
  std::string conffile;
  std::string fifo;
  int c;
  char* name = strrchr(argc[0], '/');
  name = name ? name + 1 : argc[0];
  while((c = getopt(argv, argc, "c:f:")) != -1) {
    if(c == 'c')
      conffile = optarg;
    else if(c == 'f')
      fifo = optarg;
    else {
      std::cout << "Usage: " << name << " -c conffile -f fifo" << std::endl;
      return 1;
    }
  }
  if(optind < argv) {
    std::cout << "Usage: " << name << " -c conffile -f fifo" << std::endl;
    return 1;
  }
  if(conffile.empty()) {
    std::cout << "No configuration file" << std::endl;
    return 1;
  }
  if(fifo.empty()) {
    std::cout << "No FIFO filename" << std::endl;
    return 1;
  }
  Server s(fifo, conffile);

  int i = fork();
  if(i < 0)
    exit(1);
  if(i > 0)
    exit(0);
  setsid();
  if(freopen("/dev/null", "r", stdin) == NULL)
    fclose(stdin);
  if(freopen("/dev/null", "a", stdout) == NULL)
    fclose(stdout);
  if(freopen("/dev/null", "a", stderr) == NULL)
    fclose(stderr);

  signal(SIGPIPE, SIG_IGN);

  s.Start();

  return 0;
}
