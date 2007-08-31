// apstat.cpp

#include <iostream>
#include <fstream>
#include <string>
#include <stdexcept>
#include "arex_client.h"

//! A prototype client for job status queries.
/*! A prototype command line tool for job status queries to an A-REX
  service. In the name, "ap" means "Arc Prototype".
  
  Usage:
  apstat <JobID-file>

  Arguments:
  <JobID-file> The name of a file in which the Job ID is stored.

  Configuration:
  Which A-REX service to use is specified in a configuration file. The
  configuration file also specifies how to set up the communication
  chain for the client. The location of the configuration file is
  specified by the environment variable ARC_AREX_CONFIG. If there is
  no such environment variable, the configuration file is assumed to
  be "arex_client.xml" in the current working directory.
*/
int main(int argc, char* argv[]){
  try{
    if (argc!=2)
      throw std::invalid_argument("Wrong number of arguments!");
    Arc::AREXClient ac;
    std::string jobid;
    std::ifstream jobidfile(argv[1]);
    if (!jobidfile)
      throw std::invalid_argument(std::string("Could not open ")+
				  std::string(argv[1]));
    std::getline<char>(jobidfile, jobid, 0);
    if (!jobidfile)
      throw std::invalid_argument(std::string("Could not read Job ID from ")+
				  std::string(argv[2]));
    std::cout << "Job status: " << ac.stat(jobid) << std::endl;
    return EXIT_SUCCESS;
  }
  catch (std::exception& err){
    std::cerr << "ERROR: " << err.what() << std::endl;
    return EXIT_FAILURE;
  }
}
