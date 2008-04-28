#include "Logger.h"
#include "Run.h"
#include "Thread.h"
#include <string>
#include <iostream>
#if WIN32
#incude <arc/win32.h>
#endif

Arc::Run *executer = NULL;

void start_thread(void *arg) 
{
    std::cout << "Start" << std::endl;
    try {
        std::string std_in;   
        std::string std_out;
        std::string std_err;

        executer = new Arc::Run("/bin/sleep 60");
    
        executer->AssignStdin(std_in);
        executer->AssignStdout(std_out);
        executer->AssignStderr(std_err);

        if (!executer->Start()) {
            std::cout << "Failed to start" << std::endl;
            return;
        };
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

void stop_thread(void *arg)
{
    sleep(10);
std::cout << "Kill" << std::endl;
    executer->Kill(1);
std::cout << "Killed" << std::endl;
}

int main(void)
{
std::cout << "main start" << std::endl;
    Arc::CreateThreadFunction(&start_thread, NULL);
    Arc::CreateThreadFunction(&stop_thread, NULL);
    sleep(100000);
std::cout << "main end" << std::endl;
    return 0;
}
