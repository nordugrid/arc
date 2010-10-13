#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string>

#include <glibmm.h>

#include <arc/Thread.h>
#include <arc/Run.h>
#include <arc/Logger.h>
#include <arc/XMLNode.h>

#include "../conf/conf.h"
#include "../conf/conf_sections.h"
#include "../conf/environment.h"
#include "../jobs/users.h"
#include "../files/info_files.h"

/*
* janitor [ deploy | help | list | info | register |
*             remove | setstate | search | sweep ]
*   deploy <job-id>:             deploys all REs needed by job <job-id>
*   help:                        displays this message
*   list:                        lists details about all registered jobs
*   info <job-id>:               lists information about this job's REs
*   register <job-id> [ REs ]:   registers a new job
*   remove <job-id>:             unregister job <job-id>
*   setstate <newstate> [ REs ]: sets the state of REs
*   search [ REs ]:              searches for informations about REs
*   sweep [--force]:             clean up runtime environments
*/

#include "janitor.h"


static Arc::Logger logger(Arc::Logger::rootLogger, "Janitor Control");

Janitor::Janitor(const std::string& id,const std::string& cdir,
                 const GMEnvironment& env):
         enabled_(false),id_(id),cdir_(cdir),
         running_(false),result_(FAILED),env_(env) {
  if(!readConfig()) return;
  if(!enabled()) return;
  if(id_.empty()) return;
  if(cdir_.empty()) return;
  // create path to janitor utility
  path_ = Glib::build_filename(env.nordugrid_libexec_loc(),"janitor");
  if(path_.empty()) {
    logger.msg(Arc::ERROR, "Failed to create path to janitor at %s",env.nordugrid_libexec_loc());
    return;
  };
  // check if utility exists
  if(!Glib::file_test(path_,Glib::FILE_TEST_IS_EXECUTABLE)) {
    logger.msg(Arc::ERROR, "Janitor executable not found at %s",path_);
    path_.resize(0);
    return;
  };
}

Janitor::~Janitor(void) {
  cancel();;
}

bool Janitor::readConfig() {
  // open conf file
  std::ifstream cfile;
  //if(nordugrid_config_loc().empty()) read_env_vars(true);
  if(!config_open(cfile,env_)) {
    logger.msg(Arc::ERROR,"Can't open configuration file");
    return false;
  }
  switch(config_detect(cfile)) {
    case config_file_XML: {
      Arc::XMLNode cfg;
      if(!cfg.ReadFromStream(cfile)) {
        config_close(cfile);
        logger.msg(Arc::ERROR,"Can't interpret configuration file as XML");
        return false;
      }
      std::string enabled = cfg["janitor"]["enabled"];
      enabled_ = (enabled == "1");
    }; break;
    case config_file_INI: {
      ConfigSections* cf = new ConfigSections(cfile);
      cf->AddSection("janitor");
      for(;;) {
        std::string rest;
        std::string command;
        cf->ReadNext(command,rest);
        if(command.empty()) break;
        if(command == "enabled") enabled_ = (config_next_arg(rest) == "1");
      }
      delete cf;
    }; break;
    default: {
      logger.msg(Arc::ERROR,"Can't recognize type of configuration file");
      return false;
    }; break;
  };
  config_close(cfile);
  return true;
}

void Janitor::cancel(void) {
  if(!running_) return;
  if(completed_.wait(0)) return;
  cancel_.signal();
  completed_.wait();
}

void Janitor::deploy_thread(void* arg) {
  Janitor& it = *((Janitor*)arg);
  it.result_=FAILED;
    // Fetch list of REs
  JobUser user(it.env_);
  user.SetControlDir(it.cdir_);
  std::list<std::string> rtes;
  if(!job_rte_read_file(it.id_,user,rtes)) { it.completed_.signal(); return; };
  if(rtes.size() == 0) {
    it.result_=DEPLOYED;
    it.completed_.signal();
    return;
  };
  std::string cmd;
  // Make command line
  cmd = it.path_ + " register " + it.id_;
  while(rtes.size() > 0) {
    cmd += " " + *rtes.begin();
    rtes.pop_front();
  };
  // Run command
  {
    Arc::Run run(cmd);
    if(!run) {
      logger.msg(Arc::DEBUG, "Can't run %s", cmd);
      it.completed_.signal();
      return;
    };
    run.KeepStdout(true);
    run.KeepStderr(true);
    if(!run.Start()) {
      logger.msg(Arc::DEBUG, "Can't start %s", cmd);
      it.completed_.signal();
      return;
    };
    // Wait for result or cancel request
    for(;;) {
      if(run.Wait(1)) break;
      if(it.cancel_.wait(0)) { it.completed_.signal(); return; };
    };
    if(run.Result() == 0) {
      logger.msg(Arc::DEBUG, "janitor register returned 0 - no RTE needs to be deployed");
      it.result_=DEPLOYED; it.completed_.signal();
      return;
    };
    if(run.Result() == 3) {
      logger.msg(Arc::DEBUG, "janitor register returned 3 - no Janitor enabled in configuration");
      it.result_=NOTENABLED; it.completed_.signal();
      return;
    };
    if(run.Result() != 1) {
      logger.msg(Arc::ERROR, "janitor register failed with exit code %i",run.Result());
      it.completed_.signal();
      return;
    };
    logger.msg(Arc::DEBUG, "janitor register returned 1 - need to run janitor deploy");
  };
  // Make command line
  cmd = it.path_ + " deploy " + it.id_;
  // Run command
  {
    Arc::Run run(cmd);
    if(!run) {
      logger.msg(Arc::DEBUG, "Can't run %s", cmd);
      it.completed_.signal();
      return;
    };
    run.KeepStdout(true);
    run.KeepStderr(true);
    if(!run.Start()) {
      logger.msg(Arc::DEBUG, "Can't start %s", cmd);
      it.completed_.signal();
      return;
    };
    // Wait for result or cancel request
    for(;;) {
      if(run.Wait(1)) break;
      if(it.cancel_.wait(0)) { it.completed_.signal(); return; };
    };
    if(run.Result() == 3) {
      logger.msg(Arc::VERBOSE, "janitor deploy returned 3 - no Janitor enabled in configuration");
      it.result_=NOTENABLED; it.completed_.signal();
      return;
    };
    if(run.Result() != 0) {
      logger.msg(Arc::DEBUG, "janitor deploy failed with exit code %i",run.Result());
      it.completed_.signal();
      return;
    };
  };
  it.result_=DEPLOYED;
  it.completed_.signal();
  return;
}

void Janitor::remove_thread(void* arg) {
  Janitor& it = *((Janitor*)arg);
  it.result_=FAILED;
  std::string cmd;
  // Make command line
  cmd = it.path_ + " remove " + it.id_;
  // Run command
  {
    Arc::Run run(cmd);
    if(!run) {
      logger.msg(Arc::DEBUG, "Can't run %s", cmd);
      it.completed_.signal();
      return;
    };
    run.KeepStdout(true);
    run.KeepStderr(true);
    if(!run.Start()) {
      logger.msg(Arc::DEBUG, "Can't start %s", cmd);
      it.completed_.signal();
      return;
    };
    // Wait for result or cancel request
    for(;;) {
      if(run.Wait(1)) break;
      if(it.cancel_.wait(0)) { it.completed_.signal(); return; };
    };
    if(run.Result() == 3) {
      logger.msg(Arc::VERBOSE, "janitor remove returned 3 - no Janitor enabled in configuration");
      it.result_=NOTENABLED; it.completed_.signal();
      return;
    };
    if(run.Result() != 0) {
      logger.msg(Arc::DEBUG, "janitor remove failed with exit code %i",run.Result());
      it.completed_.signal();
      return;
    };
  }
  it.result_=REMOVED;
  it.completed_.signal();
  return;
}

bool Janitor::deploy(void) {
  if(operator!()) return false;
  if(running_) return false;
  running_=true;
  running_=Arc::CreateThreadFunction(&Janitor::deploy_thread,this);
  return running_;
}

bool Janitor::remove(void) {
  if(operator!()) return false;
  if(running_) return false;
  running_=true;
  running_=Arc::CreateThreadFunction(&Janitor::remove_thread,this);
  return running_;
}

bool Janitor::wait(int timeout) {
  if(!running_) return true;
  if(!completed_.wait(timeout*1000)) return false;
  running_=false;
  return true;
}

Janitor::Result Janitor::result(void) {
  return result_;
}




