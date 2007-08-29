// arex_client.h
// A very simple client class for the A-REX service.

#ifndef __AREX_CLIENT__
#define __AREX_CLIENT__

#include <stdlib.h>
#include <string>
#include <iostream>
#include <stdexcept>
#include "../../libs/common/ArcConfig.h"
#include "../../libs/common/Logger.h"
#include "../../libs/common/XMLNode.h"
#include "../../hed/libs/loader/Loader.h"
#include "../../hed/libs/message/SOAPEnvelope.h"
#include "../../hed/libs/message/PayloadSOAP.h"

namespace Arc {

  //! An exception class for the AREXClient class.
  /*! This is an exception class that is used to handle runtime errors
    discovered in the AREXClient class.
   */
  class AREXClientError : public std::runtime_error {
  public:

    //! Constructor
    /*! This is the constructor of the AREXClientError class.
      @param what An explanation of the error.
     */
    AREXClientError(const std::string& what="");
  };


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
      an A-REX client that corresponds to a specific A-REX service,
      which is specified in a configuration file. The configuration
      file also specifies how to set up the communication chain for
      the client. The location of the configuration file can be
      provided as a parameter to the method. If no such parameter is
      given, the environment variable ARC_AREX_CONFIG is assumed to
      contain the location. If there is no such environment variable,
      the configuration file is assumed to be "arex_client.xml" in the
      current working directory.
      @param configFile The location of the configuration file.
      @throw An AREXClientError object if an error occurs.
     */
    AREXClient(std::string configFile="") throw(AREXClientError);

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
      @return The Job ID of the the submitted job.
      @throw An AREXClientError object if an error occurs.      
     */
    std::string submit(std::istream& jsdl_file) throw(AREXClientError);

    //! Query the status of a job.
    /*! This method queries the A-REX service about the status of a
      job.
      @param jobid The Job ID of the job.
      @return The status of the job.
      @throw An AREXClientError object if an error occurs.
     */
    std::string stat(const std::string& jobid) throw(AREXClientError);

    //! Terminates a job.
    /*! This method sends a request to the A-REX service to terminate
      a job.
      @param jobid The Job ID of the job to terminate.
      @return The response of the termination request.
      @throw An AREXClientError object if an error occurs.
     */
    std::string kill(const std::string& jobid) throw(AREXClientError);

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
    Arc::Loader* client_loader;

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
