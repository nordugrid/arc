// apkill.cpp

#include <iostream>
#include <fstream>
#include <string>
#include <stdexcept>
#include "arex_client.h"

//! A prototype client for job termination.
/*! A prototype command line tool for terminating a job of an A-REX
  service. In the name, "ap" means "Arc Prototype".
  
  Usage:
  apkill <JobID-file>

  Arguments:
  <JobID-file> The name of a file in which the Job ID will be stored.
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
    ac.kill(jobid);
    std::cout << "The job was terminated." << std::endl;
    return EXIT_SUCCESS;
  }
  catch (std::exception& err){
    std::cerr << "ERROR: " << err.what() << std::endl;
    return EXIT_FAILURE;
  }

}
