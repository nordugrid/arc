// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "Logger.h"
#include "Run.h"
#include "Thread.h"
#include <string>
#include <iostream>
#ifdef WIN32
#include "win32.h"
#endif

#include <glibmm.h>
#include <unistd.h>
#include <signal.h>

/*
   Arc::Run *executer = NULL;

   void start_thread(void *)
   {
 #if 0
   std::cout << "Start" << std::endl;
      std::string cmdline = "test2";
      std::string working_directory = ".";
      Glib::Pid pid_;
      Glib::ArrayHandle<std::string> argv_(Glib::shell_parse_argv(cmdline));
      Glib::spawn_async_with_pipes(working_directory,
                                   argv_,
                                   Glib::SpawnFlags(0),
                                   sigc::slot<void>(),
                                                   &pid_,
                                   NULL,
                                   NULL,
                                   NULL);
   std::cout << "End" << std::endl;
 #endif
    std::cout << "Start" << std::endl;
    try {
        std::string std_in;
        std::string std_out;
        std::string std_err;

        executer = new Arc::Run("/bin/sleep 60");
        // executer = new Arc::Run("C:/msys/bin/sleep.exe 60");
        // executer = new Arc::Run("/c/msys/bin/sleep.exe 60");
        // executer = new Arc::Run("C:/ARC/arc1-new/src/hed/libs/common/PCP.exe c:/ARC/arc1-new/src/hed/libs/common/input.txt");
        // executer = new Arc::Run("C:/ARC/arc1-new/src/hed/libs/common/test2.exe");

        executer->AssignStdin(std_in);
        executer->AssignStdout(std_out);
        executer->AssignStderr(std_err);

        if (!executer->Start()) {
            std::cout << "Failed to start" << std::endl;
            return;
        };
   std::cout << "Wait" << std::endl;
        if (executer->Wait()) {
            std::cout << "End of run" << std::endl;
            std::cout << std_out << std::endl;
        } else {
            std::cout << "Timeout" << std::endl;
        }
    } catch (std::exception &e) {
        std::cout << e.what() << std::endl;
    } catch (Glib::SpawnError &e) {
        std::cout << e.what() << std::endl;
    }
    std::cout << "end of start_thread" << std::endl;
   }

   void stop_thread(void *)
   {
 #if 0
    sleep(10);
   std::cout << "Kill" << std::endl;
    executer->Kill(1);
   std::cout << "Killed" << std::endl;
 #endif
   }

   int main(void)
   {
   std::cout << "main start" << std::endl;
    Arc::CreateThreadFunction(&start_thread, NULL);
   ///    Arc::CreateThreadFunction(&stop_thread, NULL);
    for (int i = 0; i < 100; i++) {
   std::cout << "Main: " << i << std::endl;
    sleep(1);
   }
   std::cout << "main end" << std::endl;
    return 0;
   }
 */

int main(void) {
  signal(SIGTTOU, SIG_IGN);
  std::string out;
  Arc::Run run("/bin/cat /tmp/*");
  run.AssignStdout(out);
  run.KeepStderr(true);
  run.KeepStdin(true);
  sleep(10);
  run.Start();
  if (run.Wait()) {
    std::cerr << "Success" << std::endl;
    std::cout << out << std::endl;
  }
  else
    std::cerr << "Timeout" << std::endl;
  sleep(600);
  return 0;
}
