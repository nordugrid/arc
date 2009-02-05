#ifndef __AREX_CLIENT__
#define __AREX_CLIENT__

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
    AREXClient(const URL& url, const MCCConfig& cfg);

    //! The destructor.
    /*! This is the destructor. It does what destructors usually do,
       cleans up...
     */
    ~AREXClient();

    //! Submit a job.
    /*! This method submits a job to the A-REX service corresponding
       to this client instance.
       @param jsdl_file An input stream from which the JSDL file for
       the job can be read.
       @param jobid The Job ID of the the submitted job.
       @return true on success
     */
    bool submit(std::istream& jsdl_file, std::string& jobid,
		bool delegate = false);

    //! Query the status of a job.
    /*! This method queries the A-REX service about the status of a
       job.
       @param jobid The Job ID of the job.
       @param status The status of the job.
       @return true on success
     */
    bool stat(const std::string& jobid, std::string& status);

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

    ClientSOAP* SOAP(void) {
      return client;
    }

  private:

    //! The configuration.
    /*! A configuration object containing information about how to set
       up this A-REX client.
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
    NS arex_ns;

    URL rurl;

    //! A logger for the A-REX client.
    /*! This is a logger to which all logging messages from the A-REX
       client are sent.
     */
    static Logger logger;
  };

}

#endif
