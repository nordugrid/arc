#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <cstdio>
#include <fstream>
#include <pwd.h>

#include <arc/XMLNode.h>
#include <arc/ArcConfig.h>
#include <arc/DateTime.h>
#include <arc/Logger.h>
#include <arc/OptionParser.h>

#include "jobs/users.h"
#include "conf/conf_file.h"
#include "jobs/plugins.h"
#include "files/info_files.h"

void get_arex_xml(Arc::XMLNode& arex) {
  
  Arc::Config cfg(nordugrid_config_loc().c_str());
  if (!cfg) return;

  for (int i=0;; i++) {

    Arc::XMLNode node = cfg["Chain"];
    node = node["Service"][i];
    if (!node)
      return;
    if (node.Attribute("name") == "a-rex") {
      node.New(arex);
      return;
    }
  }
}

/**
 * Print info to stdout on users' jobs
 */
int main(int argc, char* argv[]) {

  // stderr destination for error messages
  Arc::LogStream logcerr(std::cerr);
  Arc::Logger::getRootLogger().addDestination(logcerr);
  Arc::Logger::getRootLogger().setThreshold(Arc::DEBUG);
  
  Arc::OptionParser options(istring(""),
                            istring("gm-jobs displays information on "
                                    "current jobs in the system."));
  
  bool long_list = false;
  options.AddOption('l', "longlist",
                    istring("display more information on each job"),
                    long_list);

  std::string my_username;
  options.AddOption('U', "username",
                    istring("pretend utility is run by user with given name"),
                    istring("name"), my_username);
  
  int my_uid = getuid();
  options.AddOption('u', "userid",
                    istring("pretend utility is run by user with given uid"),
                    istring("uid"), my_uid);
  
  std::string conf_file;
  options.AddOption('c', "conffile",
                    istring("use specified configuration file"),
                    istring("file"), conf_file);
  
  std::string control_dir;
  options.AddOption('d', "controldir",
                    istring("read information from specified control dir"),
                    istring("dir"), control_dir);
                    
  bool show_share = false;
  options.AddOption('s', "showshares",
		    istring("print summary of jobs in each transfer share"),
		    show_share);

  std::list<std::string> params = options.Parse(argc, argv);

  if (!my_username.empty()) {
    struct passwd pw_;
    struct passwd *pw;
    char buf[BUFSIZ];
    getpwnam_r(my_username.c_str(), &pw_, buf, BUFSIZ, &pw);
    if(pw == NULL) {
      std::cout<<"Unknown user: "<<my_username<<std::endl;
      return 1;
    }
    my_uid=pw->pw_uid;
  } else {
    struct passwd pw_;
    struct passwd *pw;
    char buf[BUFSIZ];
    getpwuid_r(my_uid, &pw_, buf, BUFSIZ, &pw);
    if(pw == NULL) {
      std::cout<<"Can't recognize myself."<<std::endl;
      return 1;
    }
    my_username=pw->pw_name;
  }

  if (!conf_file.empty())
    nordugrid_config_loc(conf_file);
  
  if(!read_env_vars())
    exit(1);
  
  JobUsers users;
  
  if (control_dir.empty()) {
    JobUser my_user(my_username);
    std::ifstream cfile;
    if(!config_open(cfile)) {
      std::cout<<"Can't read configuration file"<<std::endl;
      return 1;
    }
    if (config_detect(cfile) == config_file_XML) {
      // take out the element that can be passed to configure_serviced_users
      Arc::XMLNode arex;
      get_arex_xml(arex);
      if (!arex || !configure_serviced_users(arex, users, my_uid, my_username, my_user)) {
        std::cout<<"Error processing configuration."<<std::endl;
        return 1;
      }
    } else if(!configure_serviced_users(users,my_uid,my_username,my_user)) {
      std::cout<<"Error processing configuration."<<std::endl;
      return 1;
    }
    if(users.size() == 0) {
      std::cout<<"No suitable users found in configuration."<<std::endl;
      return 1;
    }
  }
  else {
    ContinuationPlugins plugins;
    JobUsers::iterator jobuser = users.AddUser(my_username, NULL, control_dir);
    JobsList *jobs = new JobsList(*jobuser,plugins); 
    (*jobuser)=jobs; 
  }

  std::cout << "Looking for current jobs" << std::endl;

  unsigned int counters[JOB_STATE_NUM];
  unsigned int counters_pending[JOB_STATE_NUM];
  for(int i=0; i<JOB_STATE_NUM; i++) {
    counters[i] = 0;
    counters_pending[i] = 0;
  }

  std::map<std::string, int> share_preparing;
  std::map<std::string, int> share_preparing_pending;
  std::map<std::string, int> share_finishing;
  std::map<std::string, int> share_finishing_pending;

  unsigned int jobs_total = 0;
  for (JobUsers::iterator user = users.begin(); user != users.end(); ++user) {
    user->get_jobs()->ScanNewJobs(false);
    for (JobsList::iterator i=user->get_jobs()->begin(); i!=user->get_jobs()->end(); ++i) {
      if(!show_share) std::cout << "Job: "<<i->get_id();
      jobs_total++;
      bool pending;
      job_state_t new_state = job_state_read_file(i->get_id(), *user, pending);
      Arc::Time job_time(job_state_time(i->get_id(), *user));
      counters[new_state]++;
      if (pending) counters_pending[new_state]++;
      if (new_state == JOB_STATE_UNDEFINED) {
        std::cout<<" : ERROR : Unrecognizable state"<<std::endl;
        continue;
      }
      JobLocalDescription job_desc;
      if (!job_local_read_file(i->get_id(), *user, job_desc)) {
        std::cout<<" : ERROR : No local information."<<std::endl;
        continue;
      }
      if (show_share){
	if(new_state == JOB_STATE_PREPARING && !pending) share_preparing[job_desc.transfershare]++;
        else if(new_state == JOB_STATE_ACCEPTED && pending) share_preparing_pending[job_desc.transfershare]++;
        else if(new_state == JOB_STATE_FINISHING) share_finishing[job_desc.transfershare]++;
        else if(new_state == JOB_STATE_INLRMS && pending) {
          std::string jobid = i->get_id();
          if (job_lrms_mark_check(jobid,*user)) share_finishing_pending[job_desc.transfershare]++;
        };
        continue;
      }
      if (!long_list) {
        std::cout<<" : "<<states_all[new_state].name<<" : "<<job_desc.DN<<" : "<<job_time.str()<<std::endl;
        continue;
      }
      std::cout<<std::endl;
      std::cout<<"\tState: "<<states_all[new_state].name;
      if (pending)
        std::cout<<" (PENDING)";
      std::cout<<std::endl;
      std::cout<<"\tModified: "<<job_time.str()<<std::endl;
      std::cout<<"\tUser: "<<job_desc.DN<<std::endl;
      if (!job_desc.localid.empty())
        std::cout<<"\tLRMS id: "<<job_desc.localid<<std::endl;
      if (!job_desc.jobname.empty()) 
        std::cout<<"\tName: "<<job_desc.jobname<<std::endl;
      if (!job_desc.clientname.empty()) 
        std::cout<<"\tFrom: "<<job_desc.clientname<<std::endl;
    }
  }
  
  if(show_share) {
    std::cout<<"\n Preparing/Pending\tTransfer share"<<std::endl;
    for (std::map<std::string, int>::iterator i = share_preparing.begin(); i != share_preparing.end(); i++) {
      std::cout<<"         "<<i->second<<"/"<<share_preparing_pending[i->first]<<"\t\t"<<i->first<<std::endl;
    }
    for (std::map<std::string, int>::iterator i = share_preparing_pending.begin(); i != share_preparing_pending.end(); i++) {
      if (share_preparing[i->first] == 0)
        std::cout<<"         0/"<<share_preparing_pending[i->first]<<"\t\t"<<i->first<<std::endl;
    }
    std::cout<<"\n Finishing/Pending\tTransfer share"<<std::endl;
    for (std::map<std::string, int>::iterator i = share_finishing.begin(); i != share_finishing.end(); i++) {
      std::cout<<"         "<<i->second<<"/"<<share_finishing_pending[i->first]<<"\t\t"<<i->first<<std::endl;
    }
    for (std::map<std::string, int>::iterator i = share_finishing_pending.begin(); i != share_finishing_pending.end(); i++) {
      if (share_finishing[i->first] == 0)
        std::cout<<"         0/"<<share_finishing_pending[i->first]<<"\t\t"<<i->first<<std::endl;
    }
    std::cout<<std::endl;
    return 0;
  }
  
  std::cout<<"Jobs total: "<<jobs_total<<std::endl;

  for (int i=0; i<JOB_STATE_UNDEFINED; i++) {
    std::cout<<" "<<states_all[i].name<<": "<<counters[i]<<" ("<<counters_pending[i]<<")"<<std::endl;
  }
  int max;
  int max_running;
  int max_processing;
  int max_processing_emergency;
  int max_down;

  JobsList::GetMaxJobs(max, max_running);
  JobsList::GetMaxJobsLoad(max_processing, max_processing_emergency, max_down);

  #undef jobs_pending
  #define jobs_pending 0
  #undef jobs_num
  #define jobs_num counters
  int accepted = JOB_NUM_ACCEPTED;
  int running = JOB_NUM_RUNNING;
  #undef jobs_num
  #define jobs_num counters_pending
  running-=JOB_NUM_RUNNING;
  
  std::cout<<" Accepted: "<<accepted<<"/"<<max<<std::endl;
  std::cout<<" Running: "<<running<<"/"<<max_running<<std::endl;
  std::cout<<" Processing: "<<
    counters[JOB_STATE_PREPARING]-counters_pending[JOB_STATE_PREPARING]<<"+"<<
    counters[JOB_STATE_FINISHING]-counters_pending[JOB_STATE_FINISHING]<<"/"<<
    max_processing<<"+"<<max_processing_emergency<<std::endl;
  
  for (JobUsers::iterator user = users.begin(); user != users.end(); ++user) {
    JobsList* jobs = user->get_jobs();
    if(jobs) delete jobs;
  }

  return 0;
}
