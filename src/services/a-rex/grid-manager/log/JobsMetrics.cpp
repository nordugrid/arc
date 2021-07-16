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

JobStateList::JobStateList(int _limit):limit(_limit){
  failures = 0;
}

JobStateList::~JobStateList(){}

JobStateList::JobNode::JobNode(bool _isfailed, std::string _job_id):
  isfailed(_isfailed),job_id(_job_id){
}

JobStateList::JobNode::~JobNode(){}


JobStateList::JobNode* JobStateList::NodeInList(std::string _job_id){
  
  std::list<JobNode>::iterator it = nodes.begin();
  while(it != nodes.end()){
      
    if(it->job_id == _job_id){
      return &(*it);
    }
    ++it;
  }
  return NULL;
}



void JobStateList::SetFailure(bool _isfailed,std::string _job_id){

  //check if the node is already in the list, and if it is update the failure status
  JobStateList::JobNode* this_node = NodeInList(_job_id);
  if(this_node){
    //existing job in the list
    if(!this_node->isfailed && _isfailed){
      //update the failure-state of the node
      //only update once (i.e. if node was not failed before)
      this_node->isfailed=_isfailed;
      failures++;
    }
  }
  else{
    JobStateList::JobNode node(_isfailed,_job_id);
    nodes.push_back(node);
    if(_isfailed)failures++;
    if(nodes.size()>limit){
      //list is now 1 too long, remove the old head of the list (oldest job)
      if(nodes.front().isfailed)failures--;
      nodes.pop_front();
    }
  }
}



JobsMetrics::JobsMetrics():enabled(false),proc(NULL),jobstatelist(100) {
  job_fail_counter = 0;
  std::memset(jobs_in_state, 0, sizeof(jobs_in_state));
  std::memset(jobs_in_state_changed, 0, sizeof(jobs_in_state_changed));
  std::memset(jobs_state_old_new, 0, sizeof(jobs_state_old_new));
  std::memset(jobs_state_old_new_changed, 0, sizeof(jobs_state_old_new_changed));
  std::memset(jobs_rate, 0, sizeof(jobs_rate));
  std::memset(jobs_rate_changed, 0, sizeof(jobs_rate_changed));

  fail_changed = false;

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


void JobsMetrics::ReportJobStateChange(const GMConfig& config,  GMJobRef i, job_state_t old_state,  job_state_t new_state) {
  if(!enabled) return; // not configured
  Glib::RecMutex::Lock lock_(lock);


  std::string job_id = i->job_id;

  /*
    ## - failed jobs -- of the last 100 jobs, the number of failed jobs
    ## - job states -- number of jobs in different A-REX internal stages
  */
  

  /*jobstatelist holds jobid and true for failed or false for non-failed job for 100 latest jobs */
  jobstatelist.SetFailure(i->CheckFailure(config),job_id);
  job_fail_counter = jobstatelist.failures;
  fail_changed = true;

  //actual states (jobstates)
  if(old_state < JOB_STATE_UNDEFINED) {
    --(jobs_in_state[old_state]);
    jobs_in_state_changed[old_state] = true;
  };
  if(new_state < JOB_STATE_UNDEFINED) {
    ++(jobs_in_state[new_state]);
    jobs_in_state_changed[new_state] = true;
  };

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

  if(fail_changed){
    if(RunMetrics(
		  std::string("AREX-JOBS-FAILED-PER-100"),
		  Arc::tostring(job_fail_counter), "int32", "failed"
		  )) {
      fail_changed = false;
      return;
    };
  }
  


  for(int state = 0; state < JOB_STATE_UNDEFINED; ++state) {
    if(jobs_in_state_changed[state]) {
      if(RunMetrics(
          std::string("AREX-JOBS-IN_STATE-") + Arc::tostring(state) + "-" + GMJob::get_state_name(static_cast<job_state_t>(state)),
          Arc::tostring(jobs_in_state[state]), "int32", "jobs"
		    )) {
        jobs_in_state_changed[state] = false;
        return;
      };
    };
  };


}

 
bool JobsMetrics::RunMetrics(const std::string name, const std::string& value, const std::string unit_type, const std::string unit) {
  if(proc) return false;
  std::list<std::string> cmd;
  if(tool_path.empty()) {
    logger.msg(Arc::ERROR,"gmetric_bin_path empty in arc.conf (should never happen the default value should be used)");
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
  cmd.push_back("arc_jobs");
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
  if(arg) {
    JobsMetrics& it = *reinterpret_cast<JobsMetrics*>(arg);
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
  // from inside process kicker (todo: redesign).
  // So do it asynchronously from another thread.
  Arc::CreateThreadFunction(&SyncAsync, arg);
}

} // namespace ARex
