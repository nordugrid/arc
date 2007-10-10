// echo_client.h
// A client class for the echo service.

#ifndef __ECHO_CLIENT__
#define __ECHO_CLIENT__

#include <stdlib.h>
#include <string>
#include <iostream>
#include <stdexcept>
#include <arc/ArcConfig.h>
#include <arc/Logger.h>
#include <arc/XMLNode.h>
#include <arc/loader/Loader.h>
#include <arc/message/SOAPEnvelope.h>
#include <arc/message/PayloadSOAP.h>

namespace Arc {

  //! An exception class for the EchoClient class.
  /*! This is an exception class that is used to handle runtime errors
    discovered in the EchoClient class.
   */
  class EchoClientError : public std::runtime_error {
  public:

    //! Constructor
    /*! This is the constructor of the EchoClientError class.
      @param what An explanation of the error.
     */
    EchoClientError(const std::string& what="");
  };


  //! A client class for the echo service.
  /*! This class is a client for the echo service - a service that
    simply returns the content of every request.
   */
  class EchoClient {
  public:

    //! The constructor for the EchoClient class.
    /*! This is the constructor for the EchoClient class. It creates
      an echo client that corresponds to a specific echo service,
      which is specified in a configuration file. The configuration
      file also specifies how to set up the communication chain for
      the client. The location of the configuration file can be
      provided as a parameter to the method. If no such parameter is
      given, the environment variable ARC_ECHO_CONFIG is assumed to
      contain the location. If there is no such environment variable,
      the configuration file is assumed to be "echo_client.xml" in the
      current working directory.
      @param configFile The location of the configuration file.
      @throw An EchoClientError object if an error occurs.
     */
    EchoClient(std::string configFile="") throw(EchoClientError);

    //! The destructor.
    /*! This is the destructor. It does what destructors usually do,
      cleans up...
     */
    ~EchoClient();
    
    //! Send an echo request.
    /*! This method sends an echo request to the echo service.
      @param jobid The Job ID of the job to terminate.
      @return The message returned by the echo service.
      @throw An AREXClientError object if an error occurs.
     */
    std::string echo(const std::string& message) throw(EchoClientError);

  private:

    //! The configuration.
    /*! A configuration object containing information about how to set
      up this echo client.
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
    Arc::NS echo_ns;

    //! A logger for the echo client.
    /*! This is a logger to which all logging messages from the echo
      client are sent.
     */
    static Arc::Logger logger;
  };

}

#endif
