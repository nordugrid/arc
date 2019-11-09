#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <cstring>
#include <map>

#include <sys/stat.h>
#include <sys/statvfs.h>

#include <arc/StringConv.h>
#include <arc/Thread.h>
#include <arc/FileUtils.h>

#include "HeartBeatMetrics.h"

#include "../conf/GMConfig.h"

namespace ARex {

static Arc::Logger& logger = Arc::Logger::getRootLogger();

HeartBeatMetrics::HeartBeatMetrics():enabled(false),proc(NULL) {
  free = 0;
  totalfree = 0;

  time_lastupdate = (time_delta = (time_now = time(NULL)));

  time_update = false;
}

HeartBeatMetrics::~HeartBeatMetrics() {
}

void HeartBeatMetrics::SetEnabled(bool val) {
  enabled = val;
}

void HeartBeatMetrics::SetConfig(const char* fname) {
  config_filename = fname;
}
  
void HeartBeatMetrics::SetGmetricPath(const char* path) {
  tool_path = path;
}


  void HeartBeatMetrics::ReportHeartBeatChange(const GMConfig& config) {
  Glib::RecMutex::Lock lock_(lock);

    struct stat st;
    std::string heartbeat_file(config.ControlDir()  + "/gm-heartbeat");
    if(Arc::FileStat(heartbeat_file, &st, true)){
      time_lastupdate = st.st_mtime;
      time_now = time(NULL);
      time_delta = time_now - time_lastupdate;
      time_update = true;
    }
    else{
      logger.msg(Arc::ERROR,"Error with hearbeatfile: %s",heartbeat_file.c_str());
      time_update = false;
    }
    
  Sync();
}

bool HeartBeatMetrics::CheckRunMetrics(void) {
  if(!proc) return true;
  if(proc->Running()) return false;
  int run_result = proc->Result();
  if(run_result != 0) {
   logger.msg(Arc::ERROR,": Metrics tool returned error code %i: %s",run_result,proc_stderr);
  };
  delete proc;
  proc = NULL;
  return true;
}

void HeartBeatMetrics::Sync(void) {
  if(!enabled) return; // not configured
  Glib::RecMutex::Lock lock_(lock);
  if(!CheckRunMetrics()) return;
  // Run gmetric to report one change at a time
  //since only one process can be started from Sync(), only 1 histogram can be sent at a time, therefore return for each call;
  //Sync is therefore called multiple times until there are not more histograms that have changed


  if(time_update){
    if(RunMetrics(
		  std::string("AREX-HEARTBEAT_LAST_SEEN"),
		  Arc::tostring(time_delta), "int32", "sec"
		  )) {
      time_update = false;
      return;
    };
  }


}

 
bool HeartBeatMetrics::RunMetrics(const std::string name, const std::string& value, const std::string unit_type, const std::string unit) {
  if(proc) return false;
  std::list<std::string> cmd;
  if(tool_path.empty()) {
    logger.msg(Arc::ERROR,"gmetric_bin_path empty in arc.conf (should never happen the default value should be used");
    return false;
  } else {
    cmd.push_back(tool_path);
  };
  if(!config_filename.empty()) {
    cmd.push_back("-c");
    cmd.push_back(config_filename);
  };
  cmd.push_back("-n");
  cmd.push_back(name);
  cmd.push_back("-g");
  cmd.push_back("arc_system");
  cmd.push_back("-v");
  cmd.push_back(value);
  cmd.push_back("-t");//unit-type
  cmd.push_back(unit_type);
  cmd.push_back("-u");//unit
  cmd.push_back(unit);
  
  proc = new Arc::Run(cmd);
  proc->AssignStderr(proc_stderr);
  proc->AssignKicker(&RunMetricsKicker, this);
  if(!(proc->Start())) {
    delete proc;
    proc = NULL;
    return false;
  };
  return true;
}

void HeartBeatMetrics::SyncAsync(void* arg) {
  HeartBeatMetrics& it = *reinterpret_cast<HeartBeatMetrics*>(arg);
  if(&it) {
    Glib::RecMutex::Lock lock_(it.lock);
    if(it.proc) {
      // Continue only if no failure in previous call.
      // Otherwise it can cause storm of failed calls.
      if(it.proc->Result() == 0) {
        it.Sync();
      };
    };
  };
}

void HeartBeatMetrics::RunMetricsKicker(void* arg) {
  // Currently it is not allowed to start new external process
  // from inside process licker (todo: redesign).
  // So do it asynchronously from another thread.
  Arc::CreateThreadFunction(&SyncAsync, arg);
}

} // namespace ARex
