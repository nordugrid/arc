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

    Arc::Run *run = NULL;
    try {
        run = new Arc::Run(cmd);
        std::string stdin_str;
        std::string stderr_str;
        std::string stdout_str;
        run->AssignStdin(stdin_str);
        run->AssignStdout(stdout_str);
        run->AssignStderr(stderr_str);
        run->AssignWorkingDirectory(wd);
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

}

}
