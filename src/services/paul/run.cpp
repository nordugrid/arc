#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <unistd.h>
#include <arc/Run.h>
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
    if (exec.empty()) {
        logger_.msg(Arc::ERROR, "Empty executable");
        return false;    
    }
    Arc::XMLNode arg;
    std::string arg_str = "";
    for (int i=0; (arg = app["Argument"][i]) != false; i++) {
        arg_str += (std::string)arg + " ";
    }
    std::string std_in = (std::string)app["Input"];
    std::string std_out = (std::string)app["Output"];
    std::string std_err = (std::string)app["Error"];
    std::string cmd;
    
    // XXX unix specific
    std::string r = job_root+"/"+j.getID();
    if (exec[0] != '/') {
        cmd = r+"/"+exec;
    } else {
        cmd = exec;
    }
    
    // XXX chdir
    if (chdir(r.c_str()) != 0) {
        logger_.msg(Arc::ERROR, "Cannot chdir");
        return false;
    }
    try {
        Arc::Run run(cmd);
        std::string stdin_str;
        std::string stderr_str;
        std::string stdout_str;
        run.AssignStdin(stdin_str);
        run.AssignStdout(stdout_str);
        run.AssignStderr(stderr_str);
        logger_.msg(Arc::DEBUG, "Command: %s", cmd);
        if(!run.Start()) {
        } else {
            logger_.msg(Arc::ERROR, "Cannot start application");
            return false;
        }
        if(!run.Wait()) {
            logger_.msg(Arc::DEBUG, "StdOut: %s", stdout_str);
            logger_.msg(Arc::DEBUG, "StdErr: %s", stderr_str);
        } else {
            logger_.msg(Arc::ERROR, "Error during the application run");
            return false;
        }
        int r = run.Result();
    } catch (std::exception &e) {
        logger_.msg(Arc::ERROR, "Exception: %s", e.what());
        return false;
    } catch (Glib::SpawnError &e) {
        logger_.msg(Arc::ERROR, "SpawnError");
        return false;
    }
    
    return true;
}

}
