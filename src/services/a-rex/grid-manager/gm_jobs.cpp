#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <cstdio>
#include <fstream>
#include <pwd.h>
#include <unistd.h>

#include <arc/DateTime.h>
#include <arc/FileUtils.h>
#include <arc/Logger.h>
#include <arc/OptionParser.h>
#include <arc/StringConv.h>

#include "conf/GMConfig.h"
#include "conf/StagingConfig.h"
#include "files/ControlFileHandling.h"
#include "jobs/CommFIFO.h"
#include "jobs/JobsList.h"
#include "../delegation/DelegationStore.h"
#include "../delegation/DelegationStores.h"

using namespace ARex;

/** Fill maps with shares taken from data staging states log */
static void get_new_data_staging_shares(const GMConfig& config,
                                        std::map<std::string, int>& share_preparing,
                                        std::map<std::string, int>& share_preparing_pending,
                                        std::map<std::string, int>& share_finishing,
                                        std::map<std::string, int>& share_finishing_pending) {
  // get DTR configuration
  StagingConfig staging_conf(config);
  if (!staging_conf) {
    std::cout<<"Could not read data staging configuration from "<<config.ConfigFile()<<std::endl;
    return;
  }
  std::string dtr_log = staging_conf.get_dtr_log();
  if (dtr_log.empty()) dtr_log = config.ControlDir()+"/dtrstate.log";

  // read DTR state info
  std::list<std::string> data;
  if (!Arc::FileRead(dtr_log, data)) {
    std::cout<<"Can't read transfer states from "<<dtr_log<<". Perhaps A-REX is not running?"<<std::endl;
    return;
  }
  // format DTR_ID state priority share [destinatinon]
  // any state but TRANSFERRING is a pending state
  for (std::list<std::string>::iterator line = data.begin(); line != data.end(); ++line) {
    std::vector<std::string> entries;
    Arc::tokenize(*line, entries, " ");
    if (entries.size() < 4 || entries.size() > 6) continue;

    std::string state = entries[1];
    std::string share = entries[3];
    bool preparing = (share.find("-download") == share.size()-9);
    if (state == "TRANSFERRING") {
      preparing ? share_preparing[share]++ : share_finishing[share]++;
    }
    else {
      preparing ? share_preparing_pending[share]++ : share_finishing_pending[share]++;
    }
  }
}

class counters_t {
 public:
  unsigned int jobs_num[JOB_STATE_NUM];
  const static unsigned int jobs_pending;
  unsigned int& operator[](int n) { return jobs_num[n]; };
};

const unsigned int counters_t::jobs_pending = 0;

static bool match_list(const std::string& arg, std::list<std::string>& args, bool erase = false) {
  for(std::list<std::string>::const_iterator a = args.begin();
                                             a != args.end(); ++a) {
    if(*a == arg) {
      //if(erase) args.erase(a);
      return true;
    }
  }
  return false;
}

/**
 * Print info to stdout on users' jobs
 */
int main(int argc, char* argv[]) {

  // stderr destination for error messages
  Arc::LogStream logcerr(std::cerr);
  Arc::Logger::getRootLogger().addDestination(logcerr);
  Arc::Logger::getRootLogger().setThreshold(Arc::DEBUG);
  
  Arc::OptionParser options(" ",
                            istring("gm-jobs displays information on "
                                    "current jobs in the system."));
  
  bool long_list = false;
  options.AddOption('l', "longlist",
                    istring("display more information on each job"),
                    long_list);

  std::string conf_file;
  options.AddOption('c', "conffile",
                    istring("use specified configuration file"),
                    istring("file"), conf_file);
  
  std::string control_dir;
  options.AddOption('d', "controldir",
                    istring("read information from specified control directory"),
                    istring("dir"), control_dir);
                    
  bool show_share = false;
  options.AddOption('s', "showshares",
		    istring("print summary of jobs in each transfer share"),
		    show_share);

  bool notshow_jobs = false;
  options.AddOption('J', "notshowjobs",
		    istring("do not print list of jobs"),
		    notshow_jobs);

  bool notshow_states = false;
  options.AddOption('S', "notshowstates",
		    istring("do not print number of jobs in each state"),
		    notshow_states);

  bool show_service = false;
  options.AddOption('w', "showservice",
		    istring("print state of the service"),
		    show_service);

  std::list<std::string> filter_users;
  options.AddOption('f', "filteruser",
		    istring("show only jobs of user(s) with specified subject name(s)"),
		    istring("dn"), filter_users);

  std::list<std::string> cancel_jobs;
  options.AddOption('k', "killjob",
                    istring("request to cancel job(s) with specified ID(s)"),
                    istring("id"), cancel_jobs);

  std::list<std::string> cancel_users;
  options.AddOption('K', "killuser",
                    istring("request to cancel jobs belonging to user(s) with specified subject name(s)"),
                    istring("dn"), cancel_users);

  std::list<std::string> clean_jobs;
  options.AddOption('r', "remjob",
                    istring("request to clean job(s) with specified ID(s)"),
                    istring("id"), clean_jobs);

  std::list<std::string> clean_users;
  options.AddOption('R', "remuser",
                    istring("request to clean jobs belonging to user(s) with specified subject name(s)"),
                    istring("dn"), clean_users);

  std::list<std::string> filter_jobs;
  options.AddOption('j', "filterjob",
                    istring("show only jobs with specified ID(s)"),
                    istring("id"), filter_jobs);

  bool show_delegs = false;
  options.AddOption('e', "listdelegs",
		    istring("print list of available delegation IDs"),
		    show_delegs);

  std::list<std::string> show_deleg_ids;
  options.AddOption('E', "showdeleg",
                    istring("print delegation token of specified ID(s)"),
                    istring("id"), show_deleg_ids);


  std::list<std::string> params = options.Parse(argc, argv);

  if(show_share) { // Why?
    notshow_jobs=true;
    notshow_states=true;
  }

  GMConfig config;
  if (!conf_file.empty())
    config.SetConfigFile(conf_file);
  
  std::cout << "Using configuration at " << config.ConfigFile() << std::endl;
  if(!config.Load()) exit(1);
  
  if (!control_dir.empty()) config.SetControlDir(control_dir);
  config.Print();

  if((!notshow_jobs) || (!notshow_states) || (show_share) ||
     (cancel_users.size() > 0) || (clean_users.size() > 0) ||
     (cancel_jobs.size() > 0) || (clean_jobs.size() > 0)) {
    std::cout << "Looking for current jobs" << std::endl;
  }

  bool service_alive = false;

  counters_t counters;
  counters_t counters_pending;

  for(int i=0; i<JOB_STATE_NUM; i++) {
    counters[i] = 0;
    counters_pending[i] = 0;
  }

  std::map<std::string, int> share_preparing;
  std::map<std::string, int> share_preparing_pending;
  std::map<std::string, int> share_finishing;
  std::map<std::string, int> share_finishing_pending;
  JobsList jobs(config);
  unsigned int jobs_total = 0;
  std::list<GMJob*> cancel_jobs_list;
  std::list<GMJob*> clean_jobs_list;

  if((!notshow_jobs) || (!notshow_states) || (show_share) ||
     (cancel_users.size() > 0) || (clean_users.size() > 0) ||
     (cancel_jobs.size() > 0) || (clean_jobs.size() > 0)) {
    if (show_share && config.UseDTR()) {
      get_new_data_staging_shares(config, share_preparing, share_preparing_pending,
                                  share_finishing, share_finishing_pending);
    }
    if(filter_jobs.size() > 0) {
      for(std::list<std::string>::iterator id = filter_jobs.begin(); id != filter_jobs.end(); ++id) {
        jobs.AddJob(*id);
      }
    } else {
      jobs.ScanAllJobs();
    }
    for (JobsList::iterator i=jobs.begin(); i!=jobs.end(); ++i) {
      // Collecting information
      bool pending;
      job_state_t new_state = job_state_read_file(i->get_id(), config, pending);
      if (new_state == JOB_STATE_UNDEFINED) {
        std::cout<<"Job: "<<i->get_id()<<" : ERROR : Unrecognizable state"<<std::endl;
        continue;
      }
      Arc::Time job_time(job_state_time(i->get_id(), config));
      jobs_total++;
      counters[new_state]++;
      if (pending) counters_pending[new_state]++;
      if (!i->GetLocalDescription(config)) {
        std::cout<<"Job: "<<i->get_id()<<" : ERROR : No local information."<<std::endl;
        continue;
      }
      JobLocalDescription& job_desc = *(i->get_local());
      if (show_share && !config.UseDTR()) {
        if(new_state == JOB_STATE_PREPARING && !pending) share_preparing[job_desc.transfershare]++;
        else if(new_state == JOB_STATE_ACCEPTED && pending) share_preparing_pending[job_desc.transfershare]++;
        else if(new_state == JOB_STATE_FINISHING) share_finishing[job_desc.transfershare]++;
        else if(new_state == JOB_STATE_INLRMS && pending) {
          std::string jobid = i->get_id();
          if (job_lrms_mark_check(jobid,config)) share_finishing_pending[job_desc.transfershare]++;
        }
      }
      if(match_list(job_desc.DN,cancel_users)) {
        cancel_jobs_list.push_back(&(*i));
      }
      if(match_list(i->get_id(),cancel_jobs)) {
        cancel_jobs_list.push_back(&(*i));
      }
      if(match_list(job_desc.DN,clean_users)) {
        clean_jobs_list.push_back(&(*i));
      }
      if(match_list(i->get_id(),clean_jobs)) {
        clean_jobs_list.push_back(&(*i));
      }
      // Printing information
      if((filter_users.size() > 0) && (!match_list(job_desc.DN,filter_users))) continue;
      if((!show_share) && (!notshow_jobs)) std::cout << "Job: "<<i->get_id();
      if(!notshow_jobs) {
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
        // TODO: print whole local
      }
    }
  }
  if(show_service) {
    if(PingFIFO(config.ControlDir())) service_alive = true;
  }
  
  if(show_share) {
    std::cout<<"\n Preparing/Pending "<<(config.UseDTR() ? "files" : "jobs ")<<"\tTransfer share"<<std::endl;
    for (std::map<std::string, int>::iterator i = share_preparing.begin(); i != share_preparing.end(); i++) {
      std::cout<<"         "<<i->second<<"/"<<share_preparing_pending[i->first]<<"\t\t\t"<<i->first<<std::endl;
    }
    for (std::map<std::string, int>::iterator i = share_preparing_pending.begin(); i != share_preparing_pending.end(); i++) {
      if (share_preparing[i->first] == 0)
        std::cout<<"         0/"<<share_preparing_pending[i->first]<<"\t\t\t"<<i->first<<std::endl;
    }
    std::cout<<"\n Finishing/Pending "<<(config.UseDTR() ? "files" : "jobs ")<<"\tTransfer share"<<std::endl;
    for (std::map<std::string, int>::iterator i = share_finishing.begin(); i != share_finishing.end(); i++) {
      std::cout<<"         "<<i->second<<"/"<<share_finishing_pending[i->first]<<"\t\t\t"<<i->first<<std::endl;
    }
    for (std::map<std::string, int>::iterator i = share_finishing_pending.begin(); i != share_finishing_pending.end(); i++) {
      if (share_finishing[i->first] == 0)
        std::cout<<"         0/"<<share_finishing_pending[i->first]<<"\t\t\t"<<i->first<<std::endl;
    }
    std::cout<<std::endl;
  }
  
  if(!notshow_states) {
    std::cout<<"Jobs total: "<<jobs_total<<std::endl;

    for (int i=0; i<JOB_STATE_UNDEFINED; i++) {
      std::cout<<" "<<states_all[i].name<<": "<<counters[i]<<" ("<<counters_pending[i]<<")"<<std::endl;
    }

    std::cout<<" Accepted: "<<jobs.AcceptedJobs()<<"/"<<config.MaxJobs()<<std::endl;
    std::cout<<" Running: "<<jobs.RunningJobs()<<"/"<<config.MaxRunning()<<std::endl;
    std::cout<<" Total: "<<jobs_total<<"/"<<config.MaxTotal()<<std::endl;
    std::cout<<" Processing: "<<
    counters[JOB_STATE_PREPARING]-counters_pending[JOB_STATE_PREPARING]<<"+"<<
    counters[JOB_STATE_FINISHING]-counters_pending[JOB_STATE_FINISHING]<<"/"<<
    config.MaxJobsStaging()<<"+"<<config.MaxStagingEmergency()<<std::endl;
  }

  if(show_delegs || (show_deleg_ids.size() > 0)) {
    ARex::DelegationStores dstores;
    ARex::DelegationStore& dstore = dstores[config.ControlDir()+"/delegations"];
    std::list<std::pair<std::string,std::string> > creds = dstore.ListCredIDs();
    for(std::list<std::pair<std::string,std::string> >::iterator cred = creds.begin();
                                     cred != creds.end(); ++cred) {
      if((filter_users.size() > 0) && (!match_list(cred->second,filter_users))) continue;
      if(show_delegs) {
        std::cout<<"Delegation: "<<cred->first<<std::endl;
        std::cout<<"\tUser: "<<cred->second<<std::endl;
      }
      if(show_deleg_ids.size() > 0) {
        // TODO: optimize to avoid full scanning.
        if(match_list(cred->first,show_deleg_ids)) {
          std::string tokenpath = dstore.FindCred(cred->first,cred->second);
          if(!tokenpath.empty()) {
            std::string token;
            if(Arc::FileRead(tokenpath,token) && (!token.empty())) {
              std::cout<<"Delegation: "<<cred->first<<", "<<cred->second<<std::endl;
              std::cout<<token<<std::endl;
            }
          }
        }
      }
    }
  }
  if(show_service) {
    std::cout<<" Service state: "<<(service_alive?"alive":"not detected")<<std::endl;
  }
  
  if(cancel_jobs_list.size() > 0) {
    for(std::list<GMJob*>::iterator job = cancel_jobs_list.begin();
                            job != cancel_jobs_list.end(); ++job) {
      if(!job_cancel_mark_put(**job, config)) {
        std::cout<<"Job: "<<(*job)->get_id()<<" : ERROR : Failed to put cancel mark"<<std::endl;
      } else {
        std::cout<<"Job: "<<(*job)->get_id()<<" : Cancel request put"<<std::endl;
      }
    }
  }
  if(clean_jobs_list.size() > 0) {
    for(std::list<GMJob*>::iterator job = clean_jobs_list.begin();
                            job != clean_jobs_list.end(); ++job) {
      bool pending;
      job_state_t new_state = job_state_read_file((*job)->get_id(), config, pending);
      if((new_state == JOB_STATE_FINISHED) || (new_state == JOB_STATE_DELETED)) {
        if(!job_clean_final(**job, config)) {
          std::cout<<"Job: "<<(*job)->get_id()<<" : ERROR : Failed to clean"<<std::endl;
        } else {
          std::cout<<"Job: "<<(*job)->get_id()<<" : Cleaned"<<std::endl;
        }
      } else {
        if(!job_clean_mark_put(**job, config)) {
          std::cout<<"Job: "<<(*job)->get_id()<<" : ERROR : Failed to put clean mark"<<std::endl;
        } else {
          std::cout<<"Job: "<<(*job)->get_id()<<" : Clean request put"<<std::endl;
        }
      }
    }
  }
  return 0;
}
