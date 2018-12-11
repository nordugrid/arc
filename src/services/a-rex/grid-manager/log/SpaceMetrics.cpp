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

#include "SpaceMetrics.h"

#include "../conf/GMConfig.h"

namespace ARex {

static Arc::Logger& logger = Arc::Logger::getRootLogger();

SpaceMetrics::SpaceMetrics():enabled(false),proc(NULL) {


  freeCache = 0;
  totalFreeCache = 0;
  freeCache_update = false;

}

SpaceMetrics::~SpaceMetrics() {
}

void SpaceMetrics::SetEnabled(bool val) {
  enabled = val;
}

void SpaceMetrics::SetConfig(const char* fname) {
  config_filename = fname;
}
  
void SpaceMetrics::SetGmetricPath(const char* path) {
  tool_path = path;
}


  void SpaceMetrics::ReportSpaceChange(const GMConfig& config) {
  Glib::RecMutex::Lock lock_(lock);


  totalFreeCache = 0;

  std::vector <std::string> cachedirs = config.CacheParams().getCacheDirs();
  if(!cachedirs.empty()){

    std::vector<std::string>::iterator it = cachedirs.begin();
    for(std::vector<std::string>::iterator i = cachedirs.begin(); i!= cachedirs.end(); i++){


      std::string path = *i;
      //cachedir can have several options, extract the path part
      if ((*i).find(" ") != std::string::npos){
	path = (*i).substr((*i).find_last_of(" ")+1, (*i).length()-(*i).find_last_of(" ")+1);
      }
      

      struct statvfs info;
      if (statvfs(path.c_str(), &info) != 0) {
	  logger.msg(Arc::ERROR,"Error getting info from statvfs for the path %s:", path);
      }
      else{
	
	// return free space in GB
	freeCache = (float)(info.f_bfree * info.f_bsize) / (float)(1024 * 1024 * 1024);
	totalFreeCache += freeCache;
	logger.msg(Arc::DEBUG, "Cache %s: Free space %f GB", path, freeCache);
	
	freeCache_update = true;
      }
    }
  }
  else{
    logger.msg(Arc::DEBUG,"No cachedirs found/configured for calculation of free space.");    
  }
  


  Sync();
}

bool SpaceMetrics::CheckRunMetrics(void) {
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

void SpaceMetrics::Sync(void) {
  if(!enabled) return; // not configured
  Glib::RecMutex::Lock lock_(lock);
  if(!CheckRunMetrics()) return;
  // Run gmetric to report one change at a time
  //since only one process can be started from Sync(), only 1 histogram can be sent at a time, therefore return for each call;
  //Sync is therefore called multiple times until there are not more histograms that have changed


  if(freeCache_update){
    if(RunMetrics(
		  std::string("AREX-CACHE-FREE"),
		  Arc::tostring(totalFreeCache), "int32", "GB"
		  )) {
      freeCache_update = false;
      return;
    };
  }


}

 
bool SpaceMetrics::RunMetrics(const std::string name, const std::string& value, const std::string unit_type, const std::string unit) {
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

void SpaceMetrics::SyncAsync(void* arg) {
  SpaceMetrics& it = *reinterpret_cast<SpaceMetrics*>(arg);
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

void SpaceMetrics::RunMetricsKicker(void* arg) {
  // Currently it is not allowed to start new external process
  // from inside process licker (todo: redesign).
  // So do it asynchronously from another thread.
  Arc::CreateThreadFunction(&SyncAsync, arg);
}

} // namespace ARex
