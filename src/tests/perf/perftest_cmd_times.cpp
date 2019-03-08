#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

// perftest_cmd.cpp

#include <unistd.h>
#include <errno.h>
#include <iostream>
#include <fstream>
#include <string>
#include <stdlib.h>
#include <glibmm/thread.h>
#include <glibmm/timer.h>
#include <vector>
#include <sys/wait.h>


// Some global shared variables...
Glib::Mutex* mutex;
int finishedProcesses;
unsigned long completedCommands;
unsigned long failedCommands;
unsigned long totalCommands;
Glib::TimeVal completedTime;
Glib::TimeVal failedTime;
Glib::TimeVal totalTime;

int numberOfProcesses;
std::string cmd_str;
std::vector<std::string> arglist;

// Round off a double to an integer.
int Round(double x){
  return int(x+0.5);
}

// Execute a command line
void execCommand() {
  // Some variables...
  Glib::TimeVal tBefore;
  Glib::TimeVal tAfter;
  char **list;
  int pid;

  list = (char **)malloc (sizeof(char *) * (arglist.size() +1));
  for (int i = 0;i < arglist.size();i++)
    list[i] = (char *)arglist[i].c_str();
  list[arglist.size()] = NULL;

  for (int i=0; i<numberOfProcesses; i++) {

    tBefore.assign_current_time();

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

      tAfter.assign_current_time();
      if(child_status != 0) {
        std::cout << "ERROR: " << cmd_str << " returns code " << child_status << std::endl;
        Glib::Mutex::Lock lock(*mutex);
        ::failedCommands++;
        ::failedTime+=tAfter-tBefore;
        finishedProcesses++;
      }
      else {
        Glib::Mutex::Lock lock(*mutex);
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
  mutex=new Glib::Mutex;

  execCommand();

  // Print the result of the test.
  Glib::Mutex::Lock lock(*mutex);
  totalCommands = completedCommands+failedCommands;
  totalTime = completedTime+failedTime;
  std::cout << "========================================" << std::endl;
  std::cout << "Number of processes: "
	    << numberOfProcesses << std::endl;
  std::cout << "Number of commands: "
	    << totalCommands << std::endl;
  std::cout << "Completed commands: "
	    << completedCommands << " ("
	    << Round(completedCommands*100.0/totalCommands)
	    << "%)" << std::endl;
  std::cout << "Failed commands: "
	    << failedCommands << " ("
	    << Round(failedCommands*100.0/totalCommands)
	    << "%)" << std::endl;
  std::cout << "Average response time for all commands: "
	    << Round(1000*totalTime.as_double()/totalCommands)
	    << " ms" << std::endl;
  if (completedCommands!=0)
    std::cout << "Average response time for completed commands: "
	      << Round(1000*completedTime.as_double()/completedCommands)
	      << " ms" << std::endl;
  if (failedCommands!=0)
    std::cout << "Average response time for failed commands: "
	      << Round(1000*failedTime.as_double()/failedCommands)
	      << " ms" << std::endl;
  std::cout << "========================================" << std::endl;

  return 0;
}
