// arex_client.h
// A very simple client class for the A-REX service.

#ifndef __COMPILER_AREX_CLIENT__
#define __COMPILER_AREX_CLIENT__

#include <stdlib.h>
#include <string>
#include <iostream>
#include <stdexcept>
#include <arc/ArcConfig.h>
#include <arc/Logger.h>
#include <arc/XMLNode.h>
#include <arc/message/MCCLoader.h>
#include <arc/message/SOAPEnvelope.h>
#include <arc/message/PayloadSOAP.h>
#include <arc/client/ClientInterface.h>
#include <arc/URL.h>

namespace Arc {

  //! An exception class for the Compiler_AREXClient class.
  /*! This is an exception class that is used to handle runtime errors
    discovered in the Compiler_AREXClient class.
   */
  class Compiler_AREXClientError : public std::runtime_error {
  public:

    //! Constructor
    /*! This is the constructor of the Compiler_AREXClientError class.
      @param what An explanation of the error.
     */
    Compiler_AREXClientError(const std::string& what="");
  };


  class Compiler_AREXFile {
  public:
    std::string remote;
    std::string local;
    Compiler_AREXFile(void) { };
    Compiler_AREXFile(const std::string& remote_,const std::string& local_):
             remote(remote_),local(local_) { };
  };

  typedef std::list<Compiler_AREXFile> Compiler_AREXFileList;

  //! A client class for the A-REX service.
  /*! This class is a client for the A-REX service (Arc
    Resource-coupled EXecution service). It provides methods for three
    operations on an A-REX service:
    - Job submission
    - Job status queries
    - Job termination
   */
  class Compiler_AREXClient {
  public:

    //! The constructor for the Compiler_AREXClient class.
    /*! This is the constructor for the Compiler_AREXClient class. It creates
      an A-REX client that corresponds to a specific A-REX service,
      which is specified in a configuration file. The configuration
      file also specifies how to set up the communication chain for
      the client. The location of the configuration file can be
      provided as a parameter to the method. If no such parameter is
      given, the environment variable ARC_Compiler_AREX_CONFIG is assumed to
      contain the location. If there is no such environment variable,
      the configuration file is assumed to be "arex_client.xml" in the
      current working directory.
      @param configFile The location of the configuration file.
      @throw An Compiler_AREXClientError object if an error occurs.
     */
    Compiler_AREXClient(std::string configFile="") throw(Compiler_AREXClientError);
    Compiler_AREXClient(const Arc::URL& url,const Arc::MCCConfig& cfg) throw(Compiler_AREXClientError);

    //! The destructor.
    /*! This is the destructor. It does what destructors usually do,
      cleans up...
     */
    ~Compiler_AREXClient();
    
    //! Submit a job.
    /*! This method submits a job to the A-REX service corresponding
      to this client instance.
      @param jsdl_file An input stream from which the JSDL file for
      the job can be read.
      @return The Job ID of the the submitted job.
      @throw An Compiler_AREXClientError object if an error occurs.      
     */
    std::string submit(std::istream& jsdl_file,Compiler_AREXFileList& file_list,bool delegate = false) throw(Compiler_AREXClientError);

    //! Query the status of a job.
    /*! This method queries the A-REX service about the status of a
      job.
      @param jobid The Job ID of the job.
      @return The status of the job.
      @throw An Compiler_AREXClientError object if an error occurs.
     */
    std::string stat(const std::string& jobid) throw(Compiler_AREXClientError);

    //! Terminates a job.
    /*! This method sends a request to the A-REX service to terminate
      a job.
      @param jobid The Job ID of the job to terminate.
      @throw An Compiler_AREXClientError object if an error occurs.
     */
    void kill(const std::string& jobid) throw(Compiler_AREXClientError);
    
    //! Removes a job.
    /*! This method sends a request to the A-REX service to remove
      a job from it's pool. If job is running it will be killed 
      by service as well.
      @param jobid The Job ID of the job to remove.
      @throw An Compiler_AREXClientError object if an error occurs.
     */
    void clean(const std::string& jobid) throw(Compiler_AREXClientError);
    
    //! Query the status of a service.
    /*! This method queries the A-REX service about it's status.
      @return The XML document representing status of the service.
      @throw An Compiler_AREXClientError object if an error occurs.
     */
    std::string sstat(void) throw(Compiler_AREXClientError);

    ClientSOAP* SOAP(void) { return client; };

  private:

    //! The configuration.
    /*! A configuration object containing information about how to set
      up this A-REX client.
     */
    Arc::Config* client_config;

    //! The loader.
    /*! A loader object that loads and connects the appropriate
      components according to the configuration object.
     */
    Arc::MCCLoader* client_loader;

    Arc::ClientSOAP* client;

    //! The entry into the client message chain.
    /*! This is a pointer to the message chain components (MCC) where
      messages sent from this client enters the message chain.
     */
    Arc::MCC* client_entry;

    //! Namespaces.
    /*! A map containing namespaces.
     */
    Arc::NS arex_ns;

    //! A logger for the A-REX client.
    /*! This is a logger to which all logging messages from the A-REX
      client are sent.
     */
    static Arc::Logger logger;
  };

}

#endif
