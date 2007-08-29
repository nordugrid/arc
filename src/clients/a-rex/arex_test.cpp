// arex_test.cpp
// A small test program that creates an A-REX client and calls its
// three methods.

#include <iostream>
#include <fstream>
#include <string>
#include "arex_client.h"

int main(){

  Arc::LogStream logcerr(std::cerr);
  Arc::Logger::rootLogger.addDestination(logcerr);
  Arc::Logger::rootLogger.setThreshold(Arc::FATAL);
  
  try{
    Arc::AREXClient ac;
    std::string jobid;
    std::ifstream jsdlfile("jsdl.xml");
    jobid = ac.submit(jsdlfile);
    std::cout << "Submitted a job with ID: " << jobid << std::endl;
    std::cout << "Status of the job: " << ac.stat(jobid) << std::endl;
    std::cout << "Killed the job: " << ac.kill(jobid) << std::endl;
    return 0;
  }
  catch (Arc::AREXClientError err){
    std::cerr << "ERROR: " << err.what() << std::endl;
    return -1;
  }

}
