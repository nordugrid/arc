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

using namespace ARex;

static Arc::Logger logger(Arc::Logger::getRootLogger(), "gm-jobs");

/** Fill maps with shares taken from data staging states log */
static void get_data_staging_shares(const GMConfig& config,
                                    std::map<std::string, int>& share_preparing,
                                    std::map<std::string, int>& share_preparing_pending,
                                    std::map<std::string, int>& share_finishing,
                                    std::map<std::string, int>& share_finishing_pending) {
  // get DTR configuration
  StagingConfig staging_conf(config);
  if (!staging_conf) {
    logger.msg(Arc::ERROR, "Could not read data staging configuration from %s", config.ConfigFile());
    return;
  }
  std::string dtr_log = staging_conf.get_dtr_log();

  // read DTR state info
  std::list<std::string> data;
  if (!Arc::FileRead(dtr_log, data)) {
    logger.msg(Arc::ERROR, "Can't read transfer states from %s. Perhaps A-REX is not running?", dtr_log);
    return;
  }
  // format DTR_ID state priority share [destination]
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
  logcerr.setFormat(Arc::LongFormat);
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

  std::list<std::string> show_deleg_jobs;
  options.AddOption('D', "showdelegjob",
                    istring("print main delegation token of specified Job ID(s)"),
                    istring("job id"), show_deleg_jobs);

  std::string output_file;
  options.AddOption('o', "output",
                    istring("output requested elements (jobs list, delegation ids and tokens) to file"),
                    istring("file name"), output_file);

  std::string debug;
  options.AddOption('x', "debug",
                    istring("FATAL, ERROR, WARNING, INFO, VERBOSE or DEBUG"),
                    istring("debuglevel"), debug);


  std::list<std::string> params = options.Parse(argc, argv);

  // If debug is specified as argument, it should be set as soon as possible
  if (!debug.empty())
    Arc::Logger::getRootLogger().setThreshold(Arc::istring_to_level(debug));

  if(show_share) { // Why?
    notshow_jobs=true;
    notshow_states=true;
  }

  GMConfig config;
  if (!conf_file.empty())
    config.SetConfigFile(conf_file);
  
  logger.msg(Arc::VERBOSE, "Using configuration at %s", config.ConfigFile());
  if(!config.Load()) exit(1);
  
  if (!control_dir.empty()) config.SetControlDir(control_dir);
  config.Print();

  DelegationStore::DbType deleg_db_type = DelegationStore::DbSQLite;
  switch(config.DelegationDBType()) {
   case GMConfig::deleg_db_bdb:
    deleg_db_type = DelegationStore::DbBerkeley;
    break;
   case GMConfig::deleg_db_sqlite:
    deleg_db_type = DelegationStore::DbSQLite;
    break;
  };

  std::ostream* outs = &std::cout;
  std::ofstream outf;
  if(!output_file.empty()) {
    outf.open(output_file.c_str());
    if(!outf.is_open()) {
      logger.msg(Arc::ERROR, "Failed to open output file '%s'", output_file);
      return -1;
    };
    outs = &outf;
  }

  if((!notshow_jobs) || (!notshow_states) || (show_share) ||
     (cancel_users.size() > 0) || (clean_users.size() > 0) ||
     (cancel_jobs.size() > 0) || (clean_jobs.size() > 0)) {
    logger.msg(Arc::VERBOSE, "Looking for current jobs");
  }

  bool service_alive = false;

  counters_t counters;
  counters_t counters_pending;

  for(int i=0; i<JOB_STATE_NUM; i++) {
    counters[i] = 0;
    counters_pending[i] = 0;
  }

  unsigned int jobs_total = 0;
  std::list<GMJob*> cancel_jobs_list;
  std::list<GMJob*> clean_jobs_list;
  std::list<GMJobRef> alljobs;

  if((!notshow_jobs) || (!notshow_states) ||
     (cancel_users.size() > 0) || (clean_users.size() > 0) ||
     (cancel_jobs.size() > 0) || (clean_jobs.size() > 0)) {
    if(filter_jobs.size() > 0) {
      for(std::list<std::string>::iterator id = filter_jobs.begin(); id != filter_jobs.end(); ++id) {
        GMJobRef jref = JobsList::GetJob(config,*id);
        if(jref) alljobs.push_back(jref);
      }
    } else {
      JobsList::GetAllJobs(config,alljobs);
    }
    for (std::list<GMJobRef>::iterator ji=alljobs.begin(); ji!=alljobs.end(); ++ji) {
      // Collecting information
      bool pending;
      GMJobRef i = *ji;
      if(!i) continue;
      job_state_t new_state = job_state_read_file(i->get_id(), config, pending);
      if (new_state == JOB_STATE_UNDEFINED) {
        logger.msg(Arc::ERROR, "Job: %s : ERROR : Unrecognizable state", i->get_id());
        continue;
      }
      Arc::Time job_time(job_state_time(i->get_id(), config));
      jobs_total++;
      counters[new_state]++;
      if (pending) counters_pending[new_state]++;
      JobLocalDescription& job_desc = *(i->GetLocalDescription(config));
      if (&job_desc == NULL) {
        logger.msg(Arc::ERROR, "Job: %s : ERROR : No local information.", i->get_id());
        continue;
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
      if((!show_share) && (!notshow_jobs)) *outs << "Job: "<<i->get_id();
      if(!notshow_jobs) {
        if (!long_list) {
          *outs<<" : "<<GMJob::get_state_name(new_state)<<" : "<<job_desc.DN<<" : "<<job_time.str()<<std::endl;
          continue;
        }
        *outs<<std::endl;
        *outs<<"\tState: "<<GMJob::get_state_name(new_state);
        if (pending) *outs<<" (PENDING)";
        *outs<<std::endl;
        *outs<<"\tModified: "<<job_time.str()<<std::endl;
        *outs<<"\tUser: "<<job_desc.DN<<std::endl;
        if (!job_desc.localid.empty())
          *outs<<"\tLRMS id: "<<job_desc.localid<<std::endl;
        if (!job_desc.jobname.empty())
          *outs<<"\tName: "<<job_desc.jobname<<std::endl;
        if (!job_desc.clientname.empty())
          *outs<<"\tFrom: "<<job_desc.clientname<<std::endl;
        // TODO: print whole local
      }
    }
  }
  if(show_service) {
    if(CommFIFO::Ping(config.ControlDir())) service_alive = true;
  }
  
  if(show_share) {
    std::map<std::string, int> share_preparing;
    std::map<std::string, int> share_preparing_pending;
    std::map<std::string, int> share_finishing;
    std::map<std::string, int> share_finishing_pending;

    get_data_staging_shares(config, share_preparing, share_preparing_pending,
                            share_finishing, share_finishing_pending);
    *outs<<"\n Preparing/Pending files\tTransfer share"<<std::endl;
    for (std::map<std::string, int>::iterator i = share_preparing.begin(); i != share_preparing.end(); i++) {
      *outs<<"         "<<i->second<<"/"<<share_preparing_pending[i->first]<<"\t\t\t"<<i->first<<std::endl;
    }
    for (std::map<std::string, int>::iterator i = share_preparing_pending.begin(); i != share_preparing_pending.end(); i++) {
      if (share_preparing[i->first] == 0)
        *outs<<"         0/"<<share_preparing_pending[i->first]<<"\t\t\t"<<i->first<<std::endl;
    }
    *outs<<"\n Finishing/Pending files\tTransfer share"<<std::endl;
    for (std::map<std::string, int>::iterator i = share_finishing.begin(); i != share_finishing.end(); i++) {
      *outs<<"         "<<i->second<<"/"<<share_finishing_pending[i->first]<<"\t\t\t"<<i->first<<std::endl;
    }
    for (std::map<std::string, int>::iterator i = share_finishing_pending.begin(); i != share_finishing_pending.end(); i++) {
      if (share_finishing[i->first] == 0)
        *outs<<"         0/"<<share_finishing_pending[i->first]<<"\t\t\t"<<i->first<<std::endl;
    }
    *outs<<std::endl;
  }
  
  if(!notshow_states) {
    *outs<<"Jobs total: "<<jobs_total<<std::endl;

    for (int i=0; i<JOB_STATE_UNDEFINED; i++) {
      *outs<<" "<<GMJob::get_state_name(static_cast<job_state_t>(i))<<": "<<counters[i]<<" ("<<counters_pending[i]<<")"<<std::endl;
    }

    unsigned int accepted = counters[JOB_STATE_ACCEPTED] +
                            counters[JOB_STATE_PREPARING] +
                            counters[JOB_STATE_SUBMITTING] +
                            counters[JOB_STATE_INLRMS] +
                            counters[JOB_STATE_FINISHING];
    *outs<<" Accepted: "<<accepted<<"/"<<config.MaxJobs()<<std::endl;
    unsigned int running = counters[JOB_STATE_SUBMITTING] +
                           counters[JOB_STATE_INLRMS];
    *outs<<" Running: "<<running<<"/"<<config.MaxRunning()<<std::endl;

    *outs<<" Total: "<<jobs_total<<"/"<<config.MaxTotal()<<std::endl;
    *outs<<" Processing: "<<
    counters[JOB_STATE_PREPARING]-counters_pending[JOB_STATE_PREPARING]<<"+"<<
    counters[JOB_STATE_FINISHING]-counters_pending[JOB_STATE_FINISHING]<<std::endl;
  }

  if(show_delegs || (show_deleg_ids.size() > 0)) {
    ARex::DelegationStore dstore(config.DelegationDir(), deleg_db_type, false);
    if(dstore) {
      std::list<std::pair<std::string,std::string> > creds = dstore.ListCredIDs();
      for(std::list<std::pair<std::string,std::string> >::iterator cred = creds.begin();
                                       cred != creds.end(); ++cred) {
        if((filter_users.size() > 0) && (!match_list(cred->second,filter_users))) continue;
        if(show_delegs) {
          *outs<<"Delegation: "<<cred->first<<std::endl;
          *outs<<"\tUser: "<<cred->second<<std::endl;
          std::list<std::string> lock_ids;
          if(dstore.GetLocks(cred->first, cred->second, lock_ids)) {
            for(std::list<std::string>::iterator lock = lock_ids.begin(); lock != lock_ids.end(); ++lock) {
              *outs<<"\tJob: "<<*lock<<std::endl;
            }
          }
        }
        if(show_deleg_ids.size() > 0) {
          // TODO: optimize to avoid full scanning.
          if(match_list(cred->first,show_deleg_ids)) {
            std::string tokenpath = dstore.FindCred(cred->first,cred->second);
            if(!tokenpath.empty()) {
              std::string token;
              if(Arc::FileRead(tokenpath,token) && (!token.empty())) {
                *outs<<"Delegation: "<<cred->first<<", "<<cred->second<<std::endl;
                *outs<<token<<std::endl;
              }
            }
          }
        }
      }
    }
  }
  if(show_deleg_jobs.size() > 0) {
    ARex::DelegationStore dstore(config.DelegationDir(), deleg_db_type, false);
    for(std::list<std::string>::iterator jobid = show_deleg_jobs.begin();
                         jobid != show_deleg_jobs.end(); ++jobid) {
      // Read job's local file to extract delegation id
      JobLocalDescription job_desc;
      if(job_local_read_file(*jobid,config,job_desc)) {
        std::string token;
        if(!job_desc.delegationid.empty())  {
          std::string tokenpath = dstore.FindCred(job_desc.delegationid,job_desc.DN);
          if(!tokenpath.empty()) {
            (void)Arc::FileRead(tokenpath,token);
          }
        }
        if(token.empty()) {
          // fall back to public only part
          (void)job_proxy_read_file(*jobid,config,token);
          job_desc.delegationid = "public";
        }
        if(!token.empty()) {
          *outs<<"Job: "<<*jobid<<std::endl;
          *outs<<"Delegation: "<<job_desc.delegationid<<", "<<job_desc.DN<<std::endl;
          *outs<<token<<std::endl;
        }
      }
    }
  }
  if(show_service) {
    *outs<<" Service state: "<<(service_alive?"alive":"not detected")<<std::endl;
  }
  
  if(cancel_jobs_list.size() > 0) {
    for(std::list<GMJob*>::iterator job = cancel_jobs_list.begin();
                            job != cancel_jobs_list.end(); ++job) {
      if(!job_cancel_mark_put(**job, config)) {
        logger.msg(Arc::ERROR, "Job: %s : ERROR : Failed to put cancel mark", (*job)->get_id());
      } else {
        logger.msg(Arc::INFO, "Job: %s : Cancel request put", (*job)->get_id());
      }
    }
  }
  if(clean_jobs_list.size() > 0) {
    for(std::list<GMJob*>::iterator job = clean_jobs_list.begin();
                            job != clean_jobs_list.end(); ++job) {
      // Do not clean job directly because it may have delegations locked.
      // Instead put clean mark and let A-REX do cleaning properly.
      if(!job_clean_mark_put(**job, config)) {
        logger.msg(Arc::ERROR, "Job: %s : ERROR : Failed to put clean mark", (*job)->get_id());
      } else {
        logger.msg(Arc::INFO, "Job: %s : Clean request put", (*job)->get_id());
      }
    }
  }
  // Cleanly destroy refrences to avoid error messags
  for (std::list<GMJobRef>::iterator ji=alljobs.begin(); ji!=alljobs.end(); ++ji) ji->Destroy();
  return 0;
}
