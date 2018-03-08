// -*- indent-tabs-mode: nil -*-

#ifndef __LOCAL_CLIENT__
#define __LOCAL_CLIENT__

#include <string>
#include <list>



#include <arc/URL.h>
#include <arc/XMLNode.h>
#include <arc/DateTime.h>
#include <arc/UserConfig.h>
#include <arc/compute/Job.h>
#include <arc/compute/JobDescription.h>


#include "../job.h"
#include "../delegation/DelegationStore.h"
#include "../delegation/DelegationStores.h"
#include "../grid-manager/jobs/GMJob.h"

/*Note to self: must check all variables if they should be public or private */

using namespace Arc;
namespace ARexLOCAL {

#define DEFAULT_JOB_RSL_MAX_SIZE (5*1024*1024)

  class LOCALClient;

  class LOCALJob {

    //private:
    
  public:

    std::string id;
    std::string state;
    std::string sessiondir;
    std::string controldir;
    std::string delegation_id;

    Arc::URL manager;
    Arc::URL resource;
    std::list<Arc::URL> stagein;
    std::list<Arc::URL> session;
    std::list<Arc::URL> stageout;
    
    LOCALJob& operator=(const Arc::Job& job);
  
    void toJob(LOCALClient* client, LOCALJob* localjob, Arc::Job& j) const;
    void toJob(LOCALClient* client, Arc::Job & job, Arc::Logger& logger) const;
   
    //added to be able to convert arexjob to LOCALJob
    LOCALJob(/*const */ARex::ARexJob& _arexjob, const ARex::GMConfig& _config, std::string const& _deleg_id);
    LOCALJob(void){};
 
  };

  //! A client class for the LOCAL service.
  /*! This class is a client for the LOCAL service. It provides methods for
     selected set of operations on a LOCAL service:
     - Job submission
     - Job status queries
     - Job termination
   */
  class LOCALClient {

  public:

    //! The constructor for the LOCALClient class.
    /*! This is the constructor for the LOCALClient class. It creates
       an LOCAL client that corresponds to a specific LOCAL service.
       @param url The URL of the LOCAL service.
       @param usercfg onfiguration object.
     */
    LOCALClient(void);//default constructur in order to use the logger
    LOCALClient(const Arc::UserConfig& usercfg);
    LOCALClient(const Arc::URL& url, const Arc::UserConfig& usercfg);
   
    //! The destructor.
    /*! This is the destructor. It does what destructors usually do,
       cleans up...
     */
    ~LOCALClient();


    Arc::URL ce;
    std::string endpoint;
    Arc::UserConfig usercfg;
    std::string cfgfile;


    //! Submit a job.
    //TO-DO Fix description 
    /*! This method submits a job to the LOCAL service corresponding
       to this client instance. It does not do data staging.
       @param jobdesc A string containing the job description.
       @param job The container for attributes identidying submitted job.
       @param state The current state of submitted job.
       @return true on success
     */

    bool submit(const std::list<Arc::JobDescription>& jobdescs, std::list<LOCALJob>& localjobs_, const std::string delegation_id = "");
    bool submit(const Arc::JobDescription& jobdesc, LOCALJob& localjob, const std::string delegation_id = "");
    bool putFiles(LOCALJob const& localjob, std::list<std::string> const& sources, std::list<std::string> const& destinations);
  

    const std::string& failure(void) {
      return lfailure;
    }

    bool info(std::list<LOCALJob>& jobids,std::list<LOCALJob>& jobids_found);
    bool info(LOCALJob& job, Arc::Job& info);
    bool clean(const std::string& jobid);
    bool kill(const std::string& jobid);
    bool restart(const std::string& jobid);
    bool list(std::list<LOCALJob>& jobs);
  
    //! Request the status of a service.
    /*! This method requests the LOCAL service about its status.
       @param status The XML document representing status of the service.
       @return true on success
     */
    bool sstat(Arc::XMLNode& xmldoc);

    Arc::User user;
    std::vector<std::string> session_dirs;
    std::vector<std::string> session_dirs_non_draining;


    ARex::GMConfig *config;
    ARex::ARexGMConfig *arexconfig;

    bool SetAndLoadConfig(ARex::GMConfig*& _config, std::string cfgfile = "");
    bool SetEndPoint(ARex::GMConfig*& _config);
    //bool SetGMDirs();
    bool PrepareARexConfig();
    //bool PreProcessJob(ARex::JobDescriptionHandler& job_desc_handler, ARex::JobLocalDescription& job_desc);
    bool CreateDelegation(std::string& deleg_id);
    bool RenewDelegation(std::string const& deleg_id);

    bool readonly;
    unsigned int job_rsl_max_size;
    bool fill_local_jobdesc(Arc::XMLNode& descr);

    std::string error_description;//should maybe be other type, check in jobplugin and relat
    std::string get_error_description() const;

    ARex::DelegationStore::DbType deleg_db_type;


    //! A logger for the A-REX client.
    /*! This is a logger to which all logging messages from the LOCAL
       client are sent.
     */
    static Arc::Logger logger;

  private:

    ARex::DelegationStores deleg_stores;
    std::list<std::string> avail_queues;

    const char* matched_vo;
    std::string lfailure;


  };

  class LOCALClients {
    std::multimap<Arc::URL, LOCALClient*> clients_;
    const Arc::UserConfig& usercfg_;
  public:
    LOCALClients(const Arc::UserConfig& usercfg);
    ~LOCALClients(void);
   
  };

}

#endif

