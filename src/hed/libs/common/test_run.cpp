#include "Run.h"
#include <string>
#include <iostream>

int main(void)
{
    try {
        std::string std_in;   
        std::string std_out;
        std::string std_err;

        Arc::Run executer("ls");
    
        executer.AssignStdin(std_in);
        executer.AssignStdout(std_out);
        executer.AssignStderr(std_err);

        if ( !executer.Start() ) {};
        if ( executer.Wait(60) ) {
            std::cout << std_out << std::endl;
        } else {
            std::cout << "Timeout" << std::endl;
        } 
    } catch (std::exception &e) {
        std::cout << e.what() << std::endl;
    }
}
