// -*- indent-tabs-mode: nil -*-

#ifndef __EMIES_CLIENT__
#define __EMIES_CLIENT__

#include <string>
#include <list>
#include <arc/URL.h>
#include <arc/XMLNode.h>
#include <arc/DateTime.h>
#include <arc/message/MCC.h>
/*
#include <utility>

#include <arc/UserConfig.h>
#include <arc/client/Job.h>
*/

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

  class EMIESJobState {
  public:
    /*
        ACCEPTED
        PREPROCESSING
        PROCESSING
        PROCESSING-ACCEPTING
        PROCESSING-QUEUED
        PROCESSING-RUNNING
        POSTPROCESSING
        TERMINAL
     */
    std::string state;
    /*
        VALIDATING
        SERVER-PAUSED
        CLIENT-PAUSED
        CLIENT-STAGEIN-POSSIBLE
        CLIENT-STAGEOUT-POSSIBLE
        PROVISIONING
        DEPROVISIONING
        SERVER-STAGEIN
        SERVER-STAGEOUT
        BATCH-SUSPEND
        APP-RUNNING
        PREPROCESSING-CANCEL
        PROCESSING-CANCEL
        POSTPROCESSING-CANCEL
        VALIDATION-FAILURE
        PREPROCESSING-FAILURE
        PROCESSING-FAILURE
        POSTPROCESSING-FAILURE
        APP-FAILURE
     */
    std::list<std::string> attributes;
    std::string description;
    Arc::Time timestamp;
    EMIESJobState& operator=(XMLNode state);
    bool operator!(void);
    bool HasAttribute(const std::string& attr) const;
  };

  class EMIESJob {
  public:
    std::string id;
    URL manager;
    URL stagein;
    URL session;
    URL stageout;
    EMIESJob& operator=(XMLNode job);
    bool operator!(void);
  };

  //! A client class for the EMI ES service.
  /*! This class is a client for the EMI ES service (European
     Middleware Initiative Execution Service). It provides methods for
     selected set of operations on an EMI ES service:
     - Job submission
     - Job status queries
     - Job termination
   */
  class EMIESClient {
  public:

    //! The constructor for the EMIESClient class.
    /*! This is the constructor for the EMIESClient class. It creates
       an EMI ES client that corresponds to a specific EMI ES service.
       @param url The URL of the EMI ES service.
       @param cfg An MCC configuration object.
     */
    EMIESClient(const URL& url, const MCCConfig& cfg, int timeout);

    //! The destructor.
    /*! This is the destructor. It does what destructors usually do,
       cleans up...
     */
    ~EMIESClient();

    //! Submit a job.
    /*! This method submits a job to the EM IES service corresponding
       to this client instance. It does not do data staging.
       @param jobdesc A string containing the job description.
       @param job The container for attributes identidying submitted job.
       @param state The current state of submitted job.
       @return true on success
     */
    bool submit(const std::string& jobdesc, EMIESJob& job, EMIESJobState& state,
                bool delegate = false);

    //! Query the status of a job.
    /*! This method queries the EMI ES service about the status of a
       job.
       @param job The Job identifier of the job.
       @param state The state of the job.
       @return true on success
     */
//    bool stat(const std::string& jobid, std::string& status);
    bool stat(const EMIESJob& job, EMIESJobState& state);

    //! Terminates a job.
    /*! This method sends a request to the EMI ES service to terminate
       a job.
       @param job The Job identifier of the job to terminate.
       @return true on success
     */
    bool kill(const EMIESJob& job);

    //! Removes a job.
    /*! This method sends a request to the EMI ES service to remove
       a job from it's pool. If job is running it will not be killed
       by service and service wil retur error.
       @param jobid The Job identifier of the job to remove.
       @return true on success
     */
    bool clean(const EMIESJob& job);

    //! Suspends a job.
    /*! This method sends a request to the EMI ES service to suspend
       a job execution if possible.
       @param jobid The Job identifier of the job to suspend.
       @return true on success
     */
    bool suspend(const EMIESJob& job);

    //! Resumes a job.
    /*! This method sends a request to the EMI ES service to resume
       a job execution if it was suspended by client request.
       @param jobid The Job identifier of the job to resume.
       @return true on success
     */
    bool resume(const EMIESJob& job);

    //! Resstarts a job.
    /*! This method sends a request to the EMI ES service to restart
       processing a job aftr failure if possible.
       @param jobid The Job identifier of the job to restart.
       @return true on success
     */
    bool restart(const EMIESJob& job);

    bool notify(const EMIESJob& job);

    //! Query the status of a service.
    /*! This method queries the EMI ES service about it's status.
       @param status The XML document representing status of the service.
       @return true on success
     */
    bool sstat(XMLNode& status);

    ClientSOAP* SOAP(void) {
      return client;
    }

    bool delegation(XMLNode& operation);

  private:
    bool process(PayloadSOAP& req, bool delegate, XMLNode& response);

    bool dosimple(const std::string& action, const std::string& id);

    ClientSOAP *client;

    //! Namespaces.
    /*! A map containing namespaces.
     */
    NS ns;

    URL rurl;

    const MCCConfig cfg;

    //! A logger for the A-REX client.
    /*! This is a logger to which all logging messages from the EMI ES
       client are sent.
     */
    static Logger logger;
  };

}

#endif

