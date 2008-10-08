#include <exception>

//#include <arc/client/JobDescription.h>
#include <arc/client/TargetGenerator.h>
#include <arc/client/UserConfig.h>
#include <arc/client/ExecutionTarget.h>
#include "RandomBroker.h"

int main() {

  std::cout << "This is a broker test."<< std::endl;
  //Arc::JobDescription jobdescr;
  
  std::string conffile;
  Arc::UserConfig usercfg(conffile);
  std::list<std::string> clusters;
  std::list<std::string> indexurls;
  
  Arc::TargetGenerator targen(usercfg, clusters, indexurls);
  Arc::RandomBroker br( targen );
  try{
      Arc::ExecutionTarget et;
        while(true){
	     br.get_Target().Print(true);
        }
  }catch(char* e) { std::cout <<"Exception: " << e <<std::endl;}

  return 0;

}
