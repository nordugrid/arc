// apsub.cpp
// A prototype command line tool for job submission to an A-REX
// service. In the name, "ap" means "Arc Prototype".

#include <iostream>
#include <fstream>
#include <string>
#include "arex_client.h"

int main(int argc, char* argv[]){

  Arc::LogStream logcerr(std::cerr);
  Arc::Logger::rootLogger.addDestination(logcerr);
  Arc::Logger::rootLogger.setThreshold(Arc::FATAL);
  
  try{
    Arc::AREXClient ac;
    std::string jobid;
    std::ifstream jsdlfile(argv[1]);
    jobid = ac.submit(jsdlfile);
    std::cout << "Submitted a job with ID: " << jobid << std::endl;
  }
  catch (Arc::AREXClientError err){
    std::cerr << "ERROR: " << err.what() << std::endl;
    return -1;
  }

}
