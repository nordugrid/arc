// -*- indent-tabs-mode: nil -*-

#ifndef __AREX_CLIENT__
#define __AREX_CLIENT__

#include <string>
#include <utility>

#include <arc/URL.h>
#include <arc/UserConfig.h>
#include <arc/XMLNode.h>
#include <arc/client/Job.h>
#include <arc/message/MCC.h>

namespace Arc {


#ifdef CPPUNITTEST
#define ClientSOAP ClientSOAPTest
#define private public
#endif


  class ClientSOAP;
  class Config;
  class Logger;
  class MCCConfig;
  class PayloadSOAP;

  //! A client class for the A-REX service.
  /*! This class is a client for the A-REX service (Arc
     Resource-coupled EXecution service). It provides methods for three
     operations on an A-REX service:
     - Job submission
     - Job status queries
     - Job termination
   */
  class AREXClient {
  public:

    //! The constructor for the AREXClient class.
    /*! This is the constructor for the AREXClient class. It creates
       an A-REX client that corresponds to a specific A-REX service.
       @param url The URL of the A-REX service.
       @param cfg An MCC configuration object.
     */
    AREXClient(const URL& url, const MCCConfig& cfg, int timeout, bool arex_features = true);

    //! The destructor.
    /*! This is the destructor. It does what destructors usually do,
       cleans up...
     */
    ~AREXClient();

    //! Submit a job.
    /*! This method submits a job to the A-REX service corresponding
       to this client instance.
       @param jobdesc A string containing the job description.
       @param jobid The Job ID of the the submitted job.
       @return true on success
     */
    bool submit(const std::string& jobdesc, std::string& jobid,
                bool delegate = false);

    //! Query the status of a job.
    /*! This method queries the A-REX service about the status of a
       job.
       @param jobid The Job ID of the job.
       @param status The status of the job.
       @return true on success
     */
//    bool stat(const std::string& jobid, std::string& status);
    bool stat(const std::string& jobid, Job& job);

    //! Terminates a job.
    /*! This method sends a request to the A-REX service to terminate
       a job.
       @param jobid The Job ID of the job to terminate.
       @return true on success
     */
    bool kill(const std::string& jobid);

    //! Removes a job.
    /*! This method sends a request to the A-REX service to remove
       a job from it's pool. If job is running it will be killed
       by service as well.
       @param jobid The Job ID of the job to remove.
       @return true on success
     */
    bool clean(const std::string& jobid);

    //! Query the status of a service.
    /*! This method queries the A-REX service about it's status.
       @param status The XML document representing status of the service.
       @return true on success
     */
    bool sstat(XMLNode& status);

    //! Query the description of a job.
    /*! This method queries the A-REX service about the description of a
       job.
       @param jobid The Job ID of the job.
       @param jobdesc The description of the job.
       @return true on success
     */
    bool getdesc(const std::string& jobid, std::string& jobdesc);


    //! Migrate a job.
    /*! This method submits a migrate request and the corresponding job
       to the AREX-service.

       @param jobid The Job ID of the job to migrate.
       @param jobdesc The job description of the job to migrate.
       @param newjobid The Job ID of returned by the AREX-client on success.
       @return true on success
     */
    bool migrate(const std::string& jobid, const std::string& jobdesc, bool forcemigration, std::string& newjobid, bool delegate = false);


    bool listServicesFromISIS(std::list< std::pair<URL, ServiceType> >& services);


    bool resume(const std::string& jobid);

    //! Create a activity identifier.
    /*! This is a convenience method to construct a activity identifier used
       in BES requests.

       @param jobid The URL of the job to construct the activity identifier from.
       @param activityIdentifier The created activity identifier will be stored
              in this object.
     */
    static void createActivityIdentifier(const URL& jobid, std::string& activityIdentifier);

    ClientSOAP* SOAP(void) {
      return client;
    }

    static const std::string mainStateModel;

    bool delegation(XMLNode& operation);
  private:
    bool process(PayloadSOAP& req, bool delegate, XMLNode& response);

    ClientSOAP *client;

    //! Namespaces.
    /*! A map containing namespaces.
     */
    NS arex_ns;

    URL rurl;

    const MCCConfig cfg;

    std::string action;

    bool arex_enabled;

    //! A logger for the A-REX client.
    /*! This is a logger to which all logging messages from the A-REX
       client are sent.
     */
    static Logger logger;
  };

}

#endif
