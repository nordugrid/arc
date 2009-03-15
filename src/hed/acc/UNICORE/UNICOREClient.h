// -*- indent-tabs-mode: nil -*-

#ifndef __UNICORE_CLIENT__
#define __UNICORE_CLIENT__

#include <string>
#include <iostream>

#include <arc/URL.h>
#include <arc/XMLNode.h>

namespace Arc {

  class ClientSOAP;
  class Config;
  class Loader;
  class Logger;
  class MCC;
  class MCCConfig;

  //! A client class for the UNICORE service.
  /*! This class implements client functionality for the UNICORE service. It provides methods for three
     operations on an UNICORE service:
     - Job submission
     - Job status queries
     - Job termination
   */
  class UNICOREClient {
  public:

    //! The constructor for the UNICOREClient class.
    /*! This is the constructor for the UNICOREClient class. It creates
       an UNICORE client that corresponds to a specific UNICORE service.
       @param url The URL of the UNICORE service.
       @param cfg An MCC configuration object.
     */
    UNICOREClient(const URL& url, const MCCConfig& cfg);

    //! The destructor.
    ~UNICOREClient();

    //! Submit a job.
    /*! This method submits a job to the UNICORE service corresponding
       to this client instance.
       @param jsdl_file An input stream from which the JSDL file for
       the job can be read.
       @param jobid The Job ID of the the submitted job.
       @return true on success
     */
    bool submit(std::istream& jsdl_file, std::string& jobid,
                bool delegate = false);

    //! Query the status of a job.
    /*! This method queries the UNICORE service about the status of a
       job.
       @param jobid The Job ID of the job.
       @param status The status of the job.
       @return true on success
     */
    bool stat(const std::string& jobid, std::string& status);

    //! Terminates a job.
    /*! This method sends a request to the UNICORE service to terminate
       a job.
       @param jobid The Job ID of the job to terminate.
       @return true on success
     */
    bool kill(const std::string& jobid);

    //! Removes a job.
    /*! This method sends a request to the UNICORE service to remove
       a job from it's pool. If job is running it will be killed
       by service as well.
       @param jobid The Job ID of the job to remove.
       @return true on success
     */
    bool clean(const std::string& jobid);

    //! Query the status of a service.
    /*! This method queries the UNICORE service about it's status.
       @param status The XML document representing status of the service.
       @return true on success
     */
    bool sstat(std::string& status);
    /*! This method queries the UNICORE registry about BES compliant
       execution services.
       @param tsf A list of different execution services returned from the
       registry. This variable will be overwritten by the method.
       @param status The XML document representing reply from the service.
       This variable will be overwritten by the method. This parameter may
       be removed at some point as it is mainly for debugging.
       @return true on success
     */
    bool listTargetSystemFactories(std::list<Arc::Config>& tsf, std::string& status);
    ClientSOAP* SOAP(void) {
      return client;
    }

  private:

    //! The configuration.
    /*! A configuration object containing information about how to set
       up this UNICORE client.
     */
    Config *client_config;

    //! The loader.
    /*! A loader object that loads and connects the appropriate
       components according to the configuration object.
     */
    Loader *client_loader;

    ClientSOAP *client;

    //! The entry into the client message chain.
    /*! This is a pointer to the message chain components (MCC) where
       messages sent from this client enters the message chain.
     */
    MCC *client_entry;

    //! Namespaces.
    /*! A map containing namespaces.
     */
    NS unicore_ns;

    URL rurl;

    //! A logger for the UNICORE client.
    /*! This is a logger to which all logging messages from the UNICORE
       client are sent.
     */
    static Logger logger;
  };

}

#endif
