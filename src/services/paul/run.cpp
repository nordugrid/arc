#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glibmm.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <arc/Run.h>
#ifdef WIN32
#include <arc/win32.h>
#else
#include <sys/wait.h>
#endif

#include "paul.h"

namespace Paul
{

bool PaulService::run(Job &j)
{
    logger_.msg(Arc::DEBUG, "Start process");
    Arc::XMLNode jd = j.getJSDL()["JobDescription"];
    Arc::XMLNode app = jd["Application"]["POSIXApplication"];
    if (app == false) {
        app = jd["Application"]["HPCProfileApplication"];
    }
    if (app == false) {
        logger_.msg(Arc::ERROR, "Invalid JSDL! Missing application section");
        return false;
    }
    std::string exec = (std::string)app["Executable"];
    // XXX protection against ../../ stuff in the executable and all kind
    // of path
    if (exec.empty()) {
        logger_.msg(Arc::ERROR, "Empty executable");
        return false;    
    }
    Arc::XMLNode arg;
    std::string arg_str = " ";
    for (int i=0; (arg = app["Argument"][i]) != false; i++) {
        arg_str += (std::string)arg + " ";
    }
    std::string std_in = (std::string)app["Input"];
    std::string std_out = (std::string)app["Output"];
    std::string std_err = (std::string)app["Error"];
    std::string cmd;
    
    // XXX unix specific
    std::string wd = Glib::build_filename(job_root, j.getID());
    mkdir(wd.c_str(), 0700);
    if (!Glib::path_is_absolute(exec)) {
        cmd = Glib::build_filename(wd, exec);
        chmod(cmd.c_str(), 0700);
#ifndef WIN32
        cmd = "./" + exec + arg_str;
#else
        cmd += (" " + arg_str);
#endif
    } else {
        cmd = exec + arg_str;
    }
    // cmd += " > s.out";
    logger_.msg(Arc::DEBUG, "Command: %s", cmd);

    
    STARTUPINFO startupinfo;
    memset(&startupinfo, 0, sizeof(startupinfo));
    // startupinfo.cb = sizeof(STARTUPINFO);
    // startupinfo.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
    // startupinfo.dwFlags = STARTF_USESHOWWINDOW | ;
    // startupinfo.wShowWindow = SW_HIDE;
    PROCESS_INFORMATION processinfo;
    
    char *cmd_s = (char *)calloc(cmd.length(), sizeof(char));
    char *wd_s = (char *)calloc(wd.length(), sizeof(char));
    strcpy(cmd_s, cmd.c_str());
    strcpy(wd_s, wd.c_str());
    // int result = CreateProcess(NULL, cmd_s, NULL, NULL, FALSE, CREATE_NEW_CONSOLE, NULL, wd_s, &startupinfo, &processinfo);
    int result = CreateProcess(NULL, 
                              (LPSTR)cmd.c_str(), 
                               NULL, 
                               NULL, 
                               FALSE, 
                               CREATE_NEW_PROCESS_GROUP|CREATE_NO_WINDOW|IDLE_PRIORITY_CLASS, 
                               NULL, 
                               (LPSTR)wd.c_str(),
                               &startupinfo, 
                               &processinfo);
    
    free(cmd_s);
    free(wd_s);
    if (!result) {
        logger_.msg(Arc::ERROR, "Spawn Error: %s", GetOsErrorMessage());
        return false;
    }
    runq[j.getID()] = processinfo.hProcess;
    j.setStatus(RUNNING);
    logger_.msg(Arc::DEBUG, "Wait for process");
    WaitForSingleObject(processinfo.hProcess, INFINITE);
    logger_.msg(Arc::DEBUG, "close process");
    WaitForSingleObject(processinfo.hThread, INFINITE);
    CloseHandle(processinfo.hThread);
    CloseHandle(processinfo.hProcess);
    logger_.msg(Arc::DEBUG, "end process");
    return true;   

#if 0
    Glib::ArrayHandle<std::string> argv(Glib::shell_parse_argv(cmd));
    Glib::Pid pid;
    try {
        Glib::spawn_async_with_pipes(r, argv, 
                             Glib::SpawnFlags(Glib::SPAWN_DO_NOT_REAP_CHILD | Glib::SPAWN_STDERR_TO_DEV_NULL | Glib::SPAWN_STDOUT_TO_DEV_NULL),
                                 sigc::slot<void>(), &pid, NULL, NULL, NULL);
        j.setStatus(RUNNING);
    } catch (Glib::Exception &e) {
        logger_.msg(Arc::ERROR, "Cannot start %s", cmd);
        return false;
    } catch (std::exception &e) {
        logger_.msg(Arc::ERROR, "Cannot start %s (%s)", cmd, e.what());
        return false;
    }
#ifndef WIN32
    int status;
    int ret = waitpid(pid, &status, 0);
    if (ret == 0) {
        logger_.msg(Arc::WARNING, "Zero return");
        return true;
    } else if (ret == -1) {
        logger_.msg(Arc::ERROR, "Error return");
        return false;
    } else {
        logger_.msg(Arc::DEBUG, "Process %s finished", ret);
        return true;
    }
#else
    WaitForSingleObject(HANDLE(pid), INFINITE);
    logger_.msg(Arc::DEBUG, "Process finished");
    return true;
#endif
    Glib::spawn_close_pid(pid);
#endif

#if 0
    Arc::Run *run = NULL;
    try {
        run = new Arc::Run(cmd);
        std::string stdin_str;
        std::string stderr_str;
        std::string stdout_str;
        run->AssignStdin(stdin_str);
        run->AssignStdout(stdout_str);
        run->AssignStderr(stderr_str);
        run->AssignWorkingDirectory(r);
        logger_.msg(Arc::DEBUG, "Command: %s", cmd);
        if(!run->Start()) {
            logger_.msg(Arc::ERROR, "Cannot start application");
            goto error;
        }
        j.setStatus(RUNNING);
        runq[j.getID()] = run;
        if(run->Wait()) {
            logger_.msg(Arc::DEBUG, "StdOut: %s", stdout_str);
            logger_.msg(Arc::DEBUG, "StdErr: %s", stderr_str);
            if (run != NULL) {
                logger_.msg(Arc::DEBUG, "delete run");
                delete run;
            }
            logger_.msg(Arc::DEBUG, "return from run");
            return true;
        } else {
            logger_.msg(Arc::ERROR, "Error during the application run");
            goto error;
        }
        int r = run->Result();
    } catch (std::exception &e) {
        logger_.msg(Arc::ERROR, "Exception: %s", e.what());
        goto error;
    } catch (Glib::SpawnError &e) {
        logger_.msg(Arc::ERROR, "SpawnError");
        goto error;
    }

error:
    if (run != NULL) {
        delete run;
    }
    return false;
#endif

}

}
