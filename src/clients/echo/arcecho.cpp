// arcecho.cpp

#include <iostream>
#include <fstream>
#include <string>
#include <stdexcept>
#include "echo_client.h"

//! A prototype client for the echo service.
/*! A prototype command line tool for the echo service. In the name,
  "ap" means "Arc Prototype".
  
  Usage:
  arcecho <message>

  Arguments:
  <message> A message that the echo service shall return back again.

  Configuration:
  Which echo service to use is specified in a configuration file. The
  configuration file also specifies how to set up the communication
  chain for the client. The location of the configuration file is
  specified by the environment variable ARC_ECHO_CONFIG. If there is
  no such environment variable, the configuration file is assumed to
  be "echo_client.xml" in the current working directory.
*/
int main(int argc, char* argv[]){
  Arc::LogStream logcerr(std::cerr);
  Arc::Logger::getRootLogger().addDestination(logcerr);
  Arc::Logger::rootLogger.setThreshold(Arc::VERBOSE);
  try{
    if (argc!=2)
      throw std::invalid_argument("Wrong number of arguments!");
    Arc::EchoClient ec;
    std::string message = argv[1];
    std::cout << ec.echo(message) << std::endl;
    return EXIT_SUCCESS;
  }
  catch (std::exception& err){
    std::cerr << "ERROR: " << err.what() << std::endl;
    return EXIT_FAILURE;
  }
}
