#ifdef HAVE_CONFIG
#include <config.h>
#endif

#include "daemon.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>
#include <iostream>
#include <errno.h>
#include <string.h>
#include <fstream>

#ifdef HAVE_SYS_FILE_H
#include <sys/file.h>
#endif /* HAVE_SYS_FILE_H */

namespace Arc {

Daemon::Daemon(std::string& pid_file)
{
    pid_t pid;
    
    pid_file = pid_file;

    pid = fork();
    switch(pid) {
        case -1: // parent fork error
            std::cerr << "daemoinzation fork failed: " << strerror(errno) << std::endl;
        case 0: // child
            /* clear inherited umasks */
            umask(0);
            /*
             * Become a session leader: setsid must succeed because child is
             * guaranteed not to be a process group leader (it belongs to the
             * process group of the parent.)
             * The goal is not to have a controlling terminal.
             * As we now don't have a controlling terminal we will not receive
             * tty-related signals - no need to ignore them.
             */ 
            setsid();
            /* close standard input */
            fclose(stdin);
            /* forward stdout and stderr to log file */
            if (std::freopen("logfile", "a", stdout) == NULL) {
                fclose(stdout);
            }
            if (std::freopen("logfile", "a", stderr) == NULL) {
                fclose(stderr);
            }
            break;
        default:
            /* write pid to pid file */
            std::fstream pf(pid_file.c_str(), std::fstream::out);
            pf << pid;
            pf.close();
            /* succesful exit from parent */
            _exit(0);
    }
}

Daemon::~Daemon() {
    // Remove pid file
    unlink(pid_file.c_str());
    // XXX: use the logging class
    std::cout << "Shoutdown daemon" << std::endl;
}

}; // namespace Arc
