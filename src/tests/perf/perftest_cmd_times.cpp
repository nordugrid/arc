#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

// perftest_cmd_times.cpp

#include <cerrno>
#include <chrono>
#include <cmath>
#include <iostream>
#include <mutex>
#include <string>
#include <vector>

#include <sys/wait.h>
#include <unistd.h>

// Some global shared variables...
std::mutex mutex;
int finishedProcesses;
unsigned long completedCommands;
unsigned long failedCommands;
unsigned long totalCommands;
std::chrono::system_clock::duration completedTime;
std::chrono::system_clock::duration failedTime;
std::chrono::system_clock::duration totalTime;

int numberOfProcesses;
std::string cmd_str;
std::vector<std::string> arglist;

// Execute a command line
void execCommand() {
  // Some variables...
  char **list;
  int pid;

  list = (char **)malloc (sizeof(char *) * (arglist.size() + 1));
  for (int i = 0;i < arglist.size();i++)
    list[i] = (char *)arglist[i].c_str();
  list[arglist.size()] = NULL;

  for (int i=0; i<numberOfProcesses; i++) {
    auto tBefore = std::chrono::system_clock::now();

    pid = fork();
    if(pid == 0) {
      int e = execvp(cmd_str.c_str(), list);
      //If execvp returns, it must have failed.
      std::cout << "[child] error " << e << " errno: " << errno << std::endl;
      exit(1);
    }
    else if(pid == -1) {
      std::cout << "Fork failed. Exiting." << std::endl;
      exit(1);
    }
    else {
      int child_status, child_pid;
      child_pid = wait(&child_status);

      auto tAfter = std::chrono::system_clock::now();

      if(child_status != 0) {
        std::cout << "ERROR: " << cmd_str << " returns code " << child_status << std::endl;
        std::unique_lock<std::mutex> lock(mutex);
        ::failedCommands++;
        ::failedTime+=tAfter-tBefore;
        finishedProcesses++;
      }
      else {
        std::unique_lock<std::mutex> lock(mutex);
        ::completedCommands++;
        ::completedTime+=tAfter-tBefore;
        finishedProcesses++;
      }
    }
  }

  std::cout << "Number of finished processes: " << finishedProcesses << std::endl;

  free(list);
}

int main(int argc, char* argv[]){
  // Some variables...
  //int numberOfProcesses;

  // Extract command line arguments.
  if (argc<3){
    std::cerr << "Wrong number of arguments!" << std::endl
              << std::endl
              << "Usage:" << std::endl
              << "perftest_cmd_times processes " << std::endl
              << std::endl
              << "Arguments:" << std::endl
              << "processes  The number of concurrent commands." << std::endl;
    exit(EXIT_FAILURE);
  }

  numberOfProcesses = atoi(argv[1]);

  cmd_str = std::string(argv[2]);
  std::cout<<"cmd_str "<<cmd_str<<std::endl;

  //Parse the arguments of command line
  for (int i = 0;i < (argc -2);i++) {
    arglist.push_back((char *)argv[i+2]);
    std::cout<<"argv: "<<arglist[i]<<std::endl;
  }

  // Start processes.
  finishedProcesses=0;

  execCommand();

  // Print the result of the test.
  std::unique_lock<std::mutex> lock(mutex);
  totalCommands = completedCommands+failedCommands;
  totalTime = completedTime+failedTime;
  std::cout << "========================================" << std::endl;
  std::cout << "Number of processes: "
            << numberOfProcesses << std::endl;
  std::cout << "Number of commands: "
            << totalCommands << std::endl;
  std::cout << "Completed commands: "
            << completedCommands << " ("
            << rint(completedCommands * 100.0 / totalCommands)
            << "%)" << std::endl;
  std::cout << "Failed commands: "
            << failedCommands << " ("
            << rint(failedCommands * 100.0 / totalCommands)
            << "%)" << std::endl;
  std::cout << "Average response time for all commands: "
            << rint(std::chrono::duration<double, std::milli>(totalTime).count() / totalCommands)
            << " ms" << std::endl;
  if (completedCommands!=0)
    std::cout << "Average response time for completed commands: "
              << rint(std::chrono::duration<double, std::milli>(completedTime).count() / completedCommands)
              << " ms" << std::endl;
  if (failedCommands!=0)
    std::cout << "Average response time for failed commands: "
              << rint(std::chrono::duration<double, std::milli>(failedTime).count() / failedCommands)
              << " ms" << std::endl;
  std::cout << "========================================" << std::endl;

  return 0;
}
