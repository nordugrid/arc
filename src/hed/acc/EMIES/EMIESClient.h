// -*- indent-tabs-mode: nil -*-

#ifndef __EMIES_CLIENT__
#define __EMIES_CLIENT__

#include <string>
#include <list>
#include <arc/URL.h>
#include <arc/XMLNode.h>
#include <arc/DateTime.h>
#include <arc/message/MCC.h>
#include <arc/UserConfig.h>
#include <arc/message/SOAPEnvelope.h>
/*
#include <utility>

#include <arc/UserConfig.h>
#include <arc/compute/Job.h>
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
  class Job;

  class EMIESJobState {
  public:
    /*
        accepted
        preprocessing
        processing
        processing-accepting
        processing-queued
        processing-running
        postprocessing
        terminal
     */
    std::string state;
    /*
        validating
        server-paused
        client-paused
        client-stagein-possible
        client-stageout-possible
        provisioning
        deprovisioning
        server-stagein
        server-stageout
        batch-suspend
        app-running
        preprocessing-cancel
        processing-cancel
        postprocessing-cancel
        validation-failure
        preprocessing-failure
        processing-failure
        postprocessing-failure
        app-failure
        expired
     */
    std::list<std::string> attributes;
    std::string description;
    Arc::Time timestamp;
    EMIESJobState& operator=(XMLNode state);
    EMIESJobState& operator=(const std::string& state);
    bool operator!(void);
    operator bool(void);
    bool HasAttribute(const std::string& attr) const;
    std::string ToXML(void) const;
  };

  class EMIESResponse {
  public:
    EMIESResponse(const EMIESResponse& r) {}
    virtual ~EMIESResponse() {}

  protected:
    EMIESResponse() {}
  };

  class EMIESJob : public EMIESResponse {
  public:
    std::string id;
    URL manager;
    URL resource;
    std::list<URL> stagein;
    std::list<URL> session;
    std::list<URL> stageout;
    EMIESJobState state;
    std::string delegation_id;
    EMIESJob& operator=(XMLNode job);
    EMIESJob& operator=(const Job& job);
    EMIESJob& operator=(const std::string& s) { XMLNode n(s); return operator=(n); }
    std::string toXML(void) const;
    void toJob(Job& j) const;
    bool operator!(void);
    operator bool(void);
    static std::string getIDFromJob(const Job*);
    static std::string getIDFromJob(const Job&);
    static std::string getIDFromJob(const EMIESJob&);
  };

  class EMIESJobInfo : public EMIESResponse {
    public:
      EMIESJobInfo(XMLNode item) : EMIESResponse() { item.New(jobInfo); }
      EMIESJobInfo(const EMIESJobInfo& ji) : EMIESResponse() {}
      void toJob(Job&) const;
      std::string getActivityID() const { return (std::string)jobInfo["ActivityID"]; }
      std::string getSubmittedVia() const;
    private:
      XMLNode jobInfo;
  };

  class EMIESAcknowledgement : public EMIESResponse {
  public:
    EMIESAcknowledgement(const std::string& id) : EMIESResponse(), activityID(id) {}
    EMIESAcknowledgement(const EMIESAcknowledgement& a) : EMIESResponse(), activityID(a.activityID) {}
    
    std::string activityID;
  };

  class EMIESFault : public EMIESResponse {
  public:
    EMIESFault() : code(0), limit(-1) {}
    std::string type;
    std::string message;
    std::string description;
    std::string activityID;
    Time timestamp;
    int code;
    int limit;
    EMIESFault& operator=(XMLNode item);
    bool operator!(void);
    operator bool(void);
    static bool isEMIESFault(XMLNode item);
    static bool isEMIESFault(XMLNode item, std::string& name);
  };
  
  class UnexpectedError : public EMIESResponse {
  public:
    UnexpectedError(const std::string& message) : EMIESResponse(), message(message) {}
    UnexpectedError(const UnexpectedError& ue) : EMIESResponse(), message(ue.message) {}
    
    const std::string message;
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

    operator bool(void) {
      return (client != NULL);
    }

    bool operator!(void) {
      return (client == NULL);
    }

    //! Submit a job.
    /*! This method submits a job to the EM IES service corresponding
       to this client instance. It does not do data staging.
       @param jobdesc A string containing the job description.
       @param job The container for attributes identidying submitted job.
       @param state The current state of submitted job.
       @return true on success
     */
    bool submit(XMLNode jobdesc, EMIESResponse** response, const std::string delegation_id = "");

    bool submit(const XMLNodeList& jobdescs, std::list<EMIESResponse*>& responses, const std::string delegation_id = "");

    //! Query the status of a job.
    /*! This method queries the EMI ES service about the status of a
       job.
       @param job The Job identifier of the job.
       @param state The state of the job.
       @return true on success
     */
    bool stat(const EMIESJob& job, XMLNode& state);
    bool stat(const EMIESJob& job, EMIESJobState& state);
    bool info(EMIESJob& job, XMLNode &state);
    bool info(EMIESJob& job, Job& info);
    template<class T>
    void info(const std::list<T>& jobs, std::list<EMIESResponse*>& responses);

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
    bool notify(const std::list<EMIESJob*> jobs, std::list<EMIESResponse*>& responses);

    //! Request the status of a service.
    /*! This method requests the EMI ES service about its status.
       @param status The XML document representing status of the service.
       @return true on success
     */
    bool sstat(XMLNode& status, bool nsapply = true);

    //! Query the status of a service.
    /*! This method queries the EMI ES service about its status using XPath 1.0.
       @param status The XML document representing status of the service.
       @return true on success
     */
    bool squery(const std::string& query, XMLNodeContainer& items, bool nsapply = true);

    //! Query the endpoints of a service.
    /*! This method queries the EMI ES service about its avaialble endpoints.
       @return true on success
     */

    bool sstat(std::list<URL>& activitycreation,
               std::list<URL>& activitymanagememt,
               std::list<URL>& activityinfo,
               std::list<URL>& resourceinfo,
               std::list<URL>& delegation);

    //! List jobs on a service.
    /*! This method queries the EMI ES service about current list of jobs.
       @param status The XML document representing status of the service.
       @return true on success
     */
    bool list(std::list<EMIESJob>& jobs);

    ClientSOAP* SOAP(void) {
      return client;
    }

    const URL& url(void) {
      return rurl;
    }

    const std::string& failure(void) {
      return lfailure;
    }

    std::string delegation(const std::string& renew_id = "");

  private:
    bool process(PayloadSOAP& req, XMLNode& response, bool retry = true);

    void process_with_vector_limit(PayloadSOAP& req, XMLNode& response);
    
    std::string dodelegation(const std::string& renew_id);

    bool reconnect(void);

    bool dosimple(const std::string& action, const std::string& id);

    EMIESClient(EMIESClient const&);
    EMIESClient& operator=(EMIESClient const&);

    ClientSOAP *client;

    //! Namespaces.
    /*! A map containing namespaces.
     */
    NS ns;

    URL rurl;

    const MCCConfig cfg;

    std::string otoken;

    int timeout;

    std::string lfailure;

    bool soapfault;

    //! A logger for the A-REX client.
    /*! This is a logger to which all logging messages from the EMI ES
       client are sent.
     */
    static Logger logger;
  };

  class EMIESClients {
    std::multimap<URL, EMIESClient*> clients_;
    const UserConfig* usercfg_;
  public:
    EMIESClients(const UserConfig& usercfg);
    ~EMIESClients(void);
    EMIESClient* acquire(const URL& url);
    void release(EMIESClient* client);
    void SetUserConfig(const UserConfig& uc);
  };

}

#endif

