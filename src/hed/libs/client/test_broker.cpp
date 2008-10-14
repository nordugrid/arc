#include <exception>

#include <arc/client/JobDescription.h>
#include <arc/client/TargetGenerator.h>
#include <arc/client/UserConfig.h>
#include <arc/client/ExecutionTarget.h>

#include "RandomBroker.h"
#include "QueueBalanceBroker.h"

int main() {

  std::cout << "This is a broker test."<< std::endl;
  
  std::string conffile = "";
  Arc::UserConfig usercfg(conffile);
  std::list<std::string> clusters;
  std::list<std::string> indexurls;
  Arc::TargetGenerator targen(usercfg, clusters, indexurls);
  Arc::JobDescription jobdescr;

  std::string source("<JobDefinition><JobDescription><Resources>The resource item</Resources><Application>The application item</Application></JobDescription></JobDefinition>");
  jobdescr.setSource(source);

  try {

        std::cout << "***************  RANDOM Broker *************" << std::endl;	
        Arc::RandomBroker br(targen, jobdescr);

        while (true){
	    	br.get_Target().Print(true);
        }	

  }

  catch (char* e) {
		std::cout << "Exception: " << e << std::endl; 
  }

  catch (char const* e) {
		std::cout << "Exception: " << e << std::endl; 
  }
    
  try {

        std::cout << "***************  QUEUEBALANCE Broker *************" << std::endl;	
        Arc::QueueBalanceBroker sr(targen, jobdescr);

        while (true){
	    	sr.get_Target().Print(true);
        }
	
  }

  catch (char *e) {
		std::cout << "Exception: " << e <<std::endl; 
  }

  catch (char const* e) {
		std::cout << "Exception: " << e << std::endl; 
  }

  return 0;
}


