#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glibmm.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <arc/Run.h>
#include <arc/StringConv.h>
#ifdef WIN32
#include <arc/win32.h>
#else
#include <sys/wait.h>
#endif

#include "paul.h"

namespace Paul
{

#ifdef WIN32
static std::string save_filename(const std::string &in)
{
    std::string out = "";
    for (int i = 0; i < in.size(); i++) {
        out += in[i];
        if ((in[i] == '\\' && i < in.size() - 1 && in[i+1] != '\\') 
            || (in[i] == '\\' && i > 0 && in[i-1] != '\\')) {
            out += in[i];
        }
    }
    return out;
}
#endif

bool PaulService::run(Job &j)
{
    logger_.msg(Arc::VERBOSE, "Start process");
    Arc::XMLNode jd = j.getJSDL()["JobDescription"];
    Arc::XMLNode app = jd["Application"]["POSIXApplication"];
    if (app == false) {
        app = jd["Application"]["HPCProfileApplication"];
    }
    if (app == false) {
        logger_.msg(Arc::ERROR, "Invalid JSDL! Missing application section");
        if (j.getStatus() != KILLED || j.getStatus() != KILLING) {
            logger_.msg(Arc::VERBOSE, "%s set exception", j.getID());
            j.setStatus(FAILED);
        }
        return false;
    }
    std::string exec = (std::string)app["Executable"];
    // XXX protection against ../../ stuff in the executable and all kind
    // of path
    if (exec.empty()) {
        logger_.msg(Arc::ERROR, "Empty executable");
        if (j.getStatus() != KILLED || j.getStatus() != KILLING) {
            logger_.msg(Arc::VERBOSE, "%s set exception", j.getID());
            j.setStatus(FAILED);
        }
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

    std::string wd = Glib::build_filename(configurator.getJobRoot(), j.getID());
    mkdir(wd.c_str(), 0700);
    if (!Glib::path_is_absolute(exec)) {
#ifdef WIN32
        size_t found = exec.find_last_of(".");
        std::string extension = exec.substr(found+1);
        extension = Arc::upper(extension);
        if (extension == "BAT") {
          std::string cmd_path = Glib::find_program_in_path("cmd");
          logger_.msg(Arc::VERBOSE, "Windows cmd path: %s", cmd_path);
          cmd = cmd_path + " /c " + exec;
        }
        if (extension != "BAT")
#endif
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
#ifdef WIN32
    cmd = save_filename(cmd);
#endif
    logger_.msg(Arc::VERBOSE, "Cmd: %s", cmd);

    Arc::Run *run = NULL;
    try {
        run = new Arc::Run(cmd);
        if (!std_in.empty())
          run->AssignStdin(std_in);
        if (!std_out.empty())
          run->AssignStdout(std_out);
        if (!std_err.empty())
          run->AssignStderr(std_err);
        run->AssignWorkingDirectory(wd);
        logger_.msg(Arc::VERBOSE, "Command: %s", cmd);
        if(!run->Start()) {
            logger_.msg(Arc::ERROR, "Cannot start application");
            goto error;
        }
        j.setStatus(RUNNING);
        runq[j.getID()] = run;
        if(run->Wait()) {
            logger_.msg(Arc::VERBOSE, "StdOut: %s", std_out);
            logger_.msg(Arc::VERBOSE, "StdErr: %s", std_err);
            if (run != NULL) {
                //logger_.msg(Arc::VERBOSE, "delete run");

                // TODO: erase should be delayed
                //runq.erase(j.getID());

                delete run;
                run = NULL;
            }
            logger_.msg(Arc::VERBOSE, "return from run");
            if (j.getStatus() != KILLED && j.getStatus() != KILLING) {
                logger_.msg(Arc::VERBOSE, "%s set finished", j.getID());
                j.setStatus(FINISHED);
            }
            return true;
        } else {
            logger_.msg(Arc::ERROR, "Error during the application run");
            goto error;
        }
        /* int r = */ run->Result();
    } catch (std::exception &e) {
        logger_.msg(Arc::ERROR, "Exception: %s", e.what());
        goto error;
    } catch (Glib::SpawnError &e) {
        logger_.msg(Arc::ERROR, "SpawnError");
        goto error;
    }

error:
    if (j.getStatus() != KILLED && j.getStatus() != KILLING) {
        logger_.msg(Arc::VERBOSE, "%s set exception", j.getID());
        j.setStatus(FAILED);
    }
    if (run != NULL) {
        runq.erase(j.getID());
        delete run;
        run = NULL;
    }
    return false;
}

}
