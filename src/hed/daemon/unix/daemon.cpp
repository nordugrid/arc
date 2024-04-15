#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>
#include <fstream>

#include <cstdio>
#include <cstdlib>
#include <cstring>
// NOTE: On Solaris errno is not working properly if cerrno is included first
#include <cerrno>

#include <assert.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>

#ifdef HAVE_SYS_FILE_H
#include <sys/file.h>
#endif /* HAVE_SYS_FILE_H */

#include <arc/Utils.h>
#include <arc/Watchdog.h>

#include "daemon.h"

#ifdef HAVE_SYSTEMD_DAEMON
#include <systemd/sd-daemon.h>
#endif

namespace Arc {

Logger Daemon::logger(Logger::rootLogger, "Daemon");


static void init_child(const std::string& log_file) {
    /* clear inherited umasks */
    ::umask(0022);
    /*
     * Become a session leader: setsid must succeed because child is
     * guaranteed not to be a process group leader (it belongs to the
     * process group of the parent.)
     * The goal is to have no controlling terminal.
     * As we now don't have a controlling terminal we will not receive
     * tty-related signals - no need to ignore them.
     */
    setsid();
    /* redirect standard input to /dev/null */
    if (std::freopen("/dev/null", "r", stdin) == NULL) fclose(stdin);
    if(!log_file.empty()) {
        /* forward stdout and stderr to log file */
        if (std::freopen(log_file.c_str(), "a", stdout) == NULL) fclose(stdout);
        if (std::freopen(log_file.c_str(), "a", stderr) == NULL) fclose(stderr);
    } else {
        if (std::freopen("/dev/null", "a", stdout) == NULL) fclose(stdout);
        if (std::freopen("/dev/null", "a", stderr) == NULL) fclose(stderr);
    }
}

static void init_parent(pid_t pid,const std::string& pid_file) {
    if(!pid_file.empty()) {
        /* write pid to pid file */
        std::fstream pf(pid_file.c_str(), std::fstream::out);
        pf << pid << std::endl;
        pf.close();
#ifdef HAVE_SYSTEMD_DAEMON
        sd_notifyf(0, "READY=1\n"
            "STATUS=Processing requests...\n"
            "MAINPID=%lu",
            (unsigned long) pid);
#endif
    }
}

Daemon::Daemon(const std::string& pid_file_, const std::string& log_file_, bool watchdog, void (*watchdog_callback)(Daemon*)) : pid_file(pid_file_),log_file(log_file_),watchdog_pid(0),watchdog_cb(watchdog_callback)
{
    pid_t pid = ::fork();
    switch(pid) {
        case -1: // parent fork error
            logger.msg(ERROR, "Daemonization fork failed: %s", StrError(errno));
            #ifdef HAVE_SYSTEMD_DAEMON
            sd_notifyf(0, "STATUS=Daemonization fork failed: %s\nERRNO=%i", StrError(errno).c_str(), errno);
            #endif
            exit(1);
        case 0: { // child
            while(true) { // stay in loop waiting for watchdog alarm
                /* And another process is left for watchdoging */
                /* Watchdog need to be initialized before fork to make sure it is shared */
                WatchdogListener wdl;
                if(watchdog) {
                    logger.msg(WARNING, "Watchdog (re)starting application");
                    pid = ::fork();
                }
                switch(pid) {
                    case -1: // parent fork error
                        logger.msg(ERROR, "Watchdog fork failed: %s", StrError(errno));
                        #ifdef HAVE_SYSTEMD_DAEMON
                        sd_notifyf(0, "STATUS=Watchdog fork failed: %s\nERRNO=%i", StrError(errno).c_str(), errno);
                        #endif
                        exit(1);
                    case 0: // real child
                        if(watchdog) watchdog_pid = ::getppid();
                        init_child(log_file);
                        break;
                    default: // watchdog
                        logger.msg(WARNING, "Watchdog starting monitoring");
                        init_parent(pid,pid_file);
                        bool error;
                        int status = 0;
                        pid_t rpid = 0;
                        for(;;) {
                            if(!wdl.Listen(30,error)) {
                                if(error) _exit(1); // watchdog gone
                                // check if child is still there
                                rpid = ::waitpid(pid,&status,WNOHANG);
                                if(rpid != 0) break;
                            } else {
                                // watchdog timeout - but check child too
                                rpid = ::waitpid(pid,&status,WNOHANG);
                                break;
                            }
                        }
                        if(watchdog_cb) {
                            // Refresh connection to log files
                            logreopen();
                            (*watchdog_cb)(this);
                        }
                        /* check if child already exited */
                        if(rpid == pid) {
                            /* child exited */
                            if(WIFSIGNALED(status)) {
                                logger.msg(WARNING, "Watchdog detected application exit due to signal %u", WTERMSIG(status));
                            } else if(WIFEXITED(status)) {
                                logger.msg(WARNING, "Watchdog detected application exited with code %u", WEXITSTATUS(status));
                            } else {
                                logger.msg(WARNING, "Watchdog detected application exit");
                            }
                            if(WIFSIGNALED(status) && 
                               ((WTERMSIG(status) == SIGSEGV) ||
                                (WTERMSIG(status) == SIGFPE) ||
                                (WTERMSIG(status) == SIGABRT) ||
                                (WTERMSIG(status) == SIGILL))) {
                            } else {
                                /* child either exited itself or was asked to */
                                logger.msg(WARNING, "Watchdog exiting because application was purposely killed or exited itself");
                                _exit(1);
                            }
                        } else if((rpid < 0) && (errno == EINTR)) {
                            // expected error - continue waiting
                        } else {
                            // Child not exited and watchdog timeouted or unexpected error - kill child process
                            logger.msg(ERROR, "Watchdog detected application timeout or error - killing process");
                            // TODO: more sophisticated killing
                            //sighandler_t old_sigterm = ::signal(SIGTERM,SIG_IGN);
                            sig_t old_sigterm = ::signal(SIGTERM,SIG_IGN);
                            int patience = 600; // how long can we wait? Maybe configure it.
                            ::kill(pid,SIGTERM);
                            while((rpid = ::waitpid(pid,&status,WNOHANG)) == 0) {
                                if(patience-- < 0) break;
                                sleep(1);
                            }
                            if(rpid != pid) {
                                logger.msg(WARNING, "Watchdog failed to wait till application exited - sending KILL");
                                // kill hardly if not exited yet or error was detected
                                ::kill(pid,SIGKILL);
                                // clean zomby
                                patience = 300; // 5 minutes should be enough
                                while(patience > 0) {
                                    --patience;
                                    rpid = ::waitpid(pid,&status,WNOHANG);
                                    if(rpid == pid) break;
                                    sleep(1);
                                }
                                if(rpid != pid) {
                                    logger.msg(WARNING, "Watchdog failed to kill application - giving up and exiting");
                                    _exit(1);
                                }
                            }
                            ::signal(SIGTERM,old_sigterm);
                        }
                        break; // go in loop and do another fork
                }
                if(pid == 0) break; // leave watchdog loop because it is child now
            }
        }; break;
        default: // original parent
            if(!watchdog) init_parent(pid,pid_file);
            /* successful exit from parent */
            _exit(0);
    }
}

Daemon::~Daemon() {
    // Remove pid file
    unlink(pid_file.c_str());
    Daemon::logger.msg(INFO, "Shutdown daemon");
}

void Daemon::logreopen(void) {
    if(!log_file.empty()) {
        if (std::freopen(log_file.c_str(), "a", stdout) == NULL) fclose(stdout);
        if (std::freopen(log_file.c_str(), "a", stderr) == NULL) fclose(stderr);
    }
}

void Daemon::shutdown(void) {
    if(watchdog_pid) kill(watchdog_pid,SIGTERM);
}

} // namespace Arc
