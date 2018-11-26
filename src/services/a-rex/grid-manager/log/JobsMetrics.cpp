#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <cstring>
#include <map>

#include <arc/StringConv.h>
#include <arc/Thread.h>

#include "JobsMetrics.h"

namespace ARex {

static Arc::Logger& logger = Arc::Logger::getRootLogger();

JobsMetrics::JobsMetrics():enabled(false),proc(NULL) {
  std::memset(jobs_processed, 0, sizeof(jobs_processed));
  std::memset(jobs_in_state, 0, sizeof(jobs_in_state));
  std::memset(jobs_processed_changed, 0, sizeof(jobs_processed_changed));
  std::memset(jobs_in_state_changed, 0, sizeof(jobs_in_state_changed));
  std::memset(jobs_state_old_new, 0, sizeof(jobs_state_old_new));
  std::memset(jobs_state_old_new_changed, 0, sizeof(jobs_state_old_new_changed));
  std::memset(jobs_rate, 0, sizeof(jobs_rate));
  std::memset(jobs_rate_changed, 0, sizeof(jobs_rate_changed));
  std::memset(jobs_state_accum, 0, sizeof(jobs_state_accum));
  std::memset(jobs_state_accum_last, 0, sizeof(jobs_state_accum_last));

  time_lastupdate = time(NULL);
}

JobsMetrics::~JobsMetrics() {
}

void JobsMetrics::SetEnabled(bool val) {
  enabled = val;
}

void JobsMetrics::SetConfig(const char* fname) {
  config_filename = fname;
}

void JobsMetrics::SetGmetricPath(const char* path) {
  tool_path = path;
}

static const char* gmetric_tool = "/usr/bin/gmetric";

void JobsMetrics::ReportJobStateChange(std::string job_id, job_state_t new_state, job_state_t old_state) {
  Glib::RecMutex::Lock lock_(lock);


  //actual states
  if(old_state < JOB_STATE_UNDEFINED) {
    ++(jobs_processed[old_state]);
    jobs_processed_changed[old_state] = true;
    --(jobs_in_state[old_state]);
    jobs_in_state_changed[old_state] = true;
  };
  if(new_state < JOB_STATE_UNDEFINED) {
    ++(jobs_in_state[new_state]);
    jobs_in_state_changed[new_state] = true;
  };


  //transitions and rates
  if((old_state <= JOB_STATE_UNDEFINED) && (new_state < JOB_STATE_UNDEFINED)){
  
    job_state_t last_old = JOB_STATE_UNDEFINED;
    job_state_t last_new = JOB_STATE_UNDEFINED;

    //find this jobs old and new state from last iteration
    if(jobs_state_old_map.find(job_id) != jobs_state_old_map.end()){
      last_old = jobs_state_old_map.find(job_id)->second;
    }
    if(jobs_state_new_map.find(job_id) != jobs_state_new_map.end()){
      last_new = jobs_state_new_map.find(job_id)->second;
    }

    if( (last_old <= JOB_STATE_UNDEFINED) && (last_new < JOB_STATE_UNDEFINED) ){
      --jobs_state_old_new[last_old][last_new];
      jobs_state_old_new_changed[last_old][last_new] = true;
    
      ++jobs_state_old_new[old_state][new_state];
      jobs_state_old_new_changed[old_state][new_state] = true;
      
      //update the old and new state jobid maps for next iteration
      std::map<std::string, job_state_t>::iterator it;
      it = jobs_state_old_map.find(job_id); 
      if (it != jobs_state_old_map.end()){
	it->second = old_state;
      }
      
      it = jobs_state_new_map.find(job_id); 
      if (it != jobs_state_new_map.end()){
	it->second = new_state;
      }
    }
    

    //for each statechange, increase number of jobs in the state and calculate rates,  at defined periods: update accum-array and histograms
    ++jobs_state_accum[new_state];
    
    time_now = time(NULL);
    time_delta = time_now - time_lastupdate;
    
    //loop over all states and caluclate rate, 
    double rate = 0.;
    for (int state = 0; state < JOB_STATE_UNDEFINED; ++state){
      if(time_delta != 0){
	rate = (static_cast<double>(jobs_state_accum[state]) - static_cast<double>(jobs_state_accum_last[state]))/time_delta;
	jobs_rate[state] = rate;
      }
      //only update histograms and values if time since last update is larger or equal defined interval
      if(time_delta >= GMETRIC_STATERATE_UPDATE_INTERVAL){
	time_lastupdate = time_now;
	jobs_state_accum_last[state] = jobs_state_accum[state];
	jobs_rate_changed[state] = true;
      }
    }
  }
  Sync();
}

bool JobsMetrics::CheckRunMetrics(void) {
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

void JobsMetrics::Sync(void) {
  if(!enabled) return; // not configured
  Glib::RecMutex::Lock lock_(lock);
  if(!CheckRunMetrics()) return;
  // Run gmetric to report one change at a time
  //since only one process can be started from Sync(), only 1 histogram can be sent at a time, therefore return for each call;
  //Sync is therefore called multiple times until there are not more histograms that have changed

  for(int state = 0; state < JOB_STATE_UNDEFINED; ++state) {
    if(jobs_processed_changed[state]) {
      if(RunMetrics(
		    std::string("AREX-JOBS-PROCESSED-") + Arc::tostring(state) + "-" + GMJob::get_state_name(static_cast<job_state_t>(state)),
		    Arc::tostring(jobs_processed[state]), "int32", "jobs"
		    )) {
        jobs_processed_changed[state] = false;
        return;
      };
    };
    if(jobs_in_state_changed[state]) {
      if(RunMetrics(
          std::string("AREX-JOBS-IN_STATE-") + Arc::tostring(state) + "-" + GMJob::get_state_name(static_cast<job_state_t>(state)),
          Arc::tostring(jobs_in_state[state]), "int32", "jobs"
		    )) {
        jobs_in_state_changed[state] = false;
        return;
      };
    };
    if(jobs_rate_changed[state]) {
      if(RunMetrics(
		    std::string("AREX-JOBS-RATE-") + Arc::tostring(state) + "-" + GMJob::get_state_name(static_cast<job_state_t>(state)),
		    Arc::tostring(jobs_rate[state]), "double", "jobs/s"
		    )) {
        jobs_rate_changed[state] = false;
        return;
      };
    };
  };

  for(int state_old = 0; state_old <= JOB_STATE_UNDEFINED; ++state_old){
    for(int state_new = 1; state_new < JOB_STATE_UNDEFINED; ++state_new){
      if(jobs_state_old_new_changed[state_old][state_new]){
  	std::string histname =  std::string("AREX-JOBS-FROM-")  + Arc::tostring(state_old) + "-" + GMJob::get_state_name(static_cast<job_state_t>(state_old)) + "-TO-"  + Arc::tostring(state_new) + "-" + GMJob::get_state_name(static_cast<job_state_t>(state_new));
  	if(RunMetrics(histname, Arc::tostring(jobs_state_old_new[state_old][state_new]), "int32", "jobs")){
  	  jobs_state_old_new_changed[state_old][state_new] = false;
  	  return;
  	};
      };
    };
  };
}
 
bool JobsMetrics::RunMetrics(const std::string name, const std::string& value, const std::string unit_type, const std::string unit) {
  if(proc) return false;
  std::list<std::string> cmd;
  if(tool_path.empty()) {
    cmd.push_back(gmetric_tool);
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

void JobsMetrics::SyncAsync(void* arg) {
  JobsMetrics& it = *reinterpret_cast<JobsMetrics*>(arg);
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

void JobsMetrics::RunMetricsKicker(void* arg) {
  // Currently it is not allowed to start new external process
  // from inside process licker (todo: redesign).
  // So do it asynchronously from another thread.
  Arc::CreateThreadFunction(&SyncAsync, arg);
}

} // namespace ARex
