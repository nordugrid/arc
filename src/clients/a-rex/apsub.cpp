// apsub.cpp

#include <iostream>
#include <fstream>
#include <string>
#include <stdexcept>
#include "arex_client.h"

//! A prototype client for job submission.
/*! A prototype command line tool for job submission to an A-REX
  service. In the name, "ap" means "Arc Prototype".
  
  Usage:
  apsub <JSDL-file> <JobID-file>

  Arguments:
  <JSDL-file> The name of a file that contains the job specification
  in JSDL format.
  <JobID-file> The name of a file in which the Job ID will be stored.

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
    if (argc!=3)
      throw std::invalid_argument("Wrong number of arguments!");
    Arc::AREXClient ac;
    std::string jobid;
    std::ifstream jsdlfile(argv[1]);
    if (!jsdlfile)
      throw std::invalid_argument(std::string("Could not open ")+
				  std::string(argv[1]));
    std::ofstream jobidfile(argv[2]);
    jobid = ac.submit(jsdlfile);
    if (!jsdlfile)
      throw std::invalid_argument(std::string("Failed when reading from ")+
				  std::string(argv[1]));
    jobidfile << jobid;
    if (!jobidfile)
      throw std::invalid_argument(std::string("Could not write Job ID to ")+
				  std::string(argv[2]));
    std::cout << "Submitted the job!" << std::endl;
    std::cout << "Job ID stored in: " << argv[2] << std::endl;
    return EXIT_SUCCESS;
  }
  catch (std::exception& e){
    std::cerr << "ERROR: " << e.what() << std::endl;
    return EXIT_FAILURE;
  }
}
