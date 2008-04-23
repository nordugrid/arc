#include <getopt.h> //Is this the best for supporting windows ?
#include <stdio.h>
#include <arc/client/TargetGenerator.h>
#include <arc/ArcConfig.h>
#include <arc/Logger.h>
#include <arc/misc/ClientInterface.h>
#include <arc/XMLNode.h>

int main(int argc, char ** argv) {

  Arc::LogStream logcerr(std::cerr);
  Arc::Logger::getRootLogger().addDestination(logcerr);
  Arc::Logger::getRootLogger().setThreshold(Arc::DEBUG);

  char * optstring = "g:c:j:";

  int opt = 0;

  Arc::ACCConfig acccfg;
  Arc::NS ns;
  Arc::Config mcfg(ns);
  acccfg.MakeConfig(mcfg);
  Arc::XMLNode AnotherOne = mcfg.NewChild("ArcClientComponent");
  char GridFlavour[20];
  char SomeURL[50];

  while(opt != -1){
    opt = getopt(argc,argv,optstring);
    if(opt == -1) continue;
    switch(opt){
    case 'g':
      std::cout<< "Arcstat called with option: -g" <<std::endl;
      std::cout<< "And argument given was:" <<optarg<<std::endl;     
      
      sscanf(optarg, "%[^:]:%s", GridFlavour, SomeURL);
 
      std::cout<< "GridFlavour:" <<GridFlavour<<std::endl;     
      std::cout<< "URL:" <<SomeURL<<std::endl;     

      if(!strcmp(GridFlavour,"ARC0")){
	AnotherOne.NewAttribute("name") = "TargetRetriever"+ (std::string) GridFlavour;
	AnotherOne.NewAttribute("id") = "retriever1";
	AnotherOne.NewChild("URL") = (std::string) SomeURL + "?giisregistrationstatus?base"; 
      }else {
	std::cout<< "Your favorite GRID flavour is not your supported" <<std::endl;     	
      }

      std::cout<< "End of case -g" <<std::endl;      
      break;
    case 'c':

      std::cout<< "Arcstat called with option: -c" <<std::endl;
      std::cout<< "And argument given was:" <<optarg<<std::endl;     
      
      sscanf(optarg, "%[^:]:%s", GridFlavour, SomeURL);
      
      std::cout<< "GridFlavour:" <<GridFlavour<<std::endl;     
      std::cout<< "URL:" <<SomeURL<<std::endl;
      if(!strcmp(GridFlavour,"ARC0")){
	AnotherOne.NewAttribute("name") = "TargetRetriever"+ (std::string) GridFlavour;
	AnotherOne.NewAttribute("id") = "retriever1";
	AnotherOne.NewChild("URL") = (std::string) SomeURL + 
                                   ",Mds-Vo-name=local,o=grid??sub?(|(objectclass=nordugrid-cluster)(objectclass=nordugrid-queue))";
 
      }else{
	std::cout<< "Your favorite GRID flavour is not your supported" <<std::endl;     	
      }

      break;
    case 'j':
      std::cout<< "arcstat called with option: -j which is not yet implemented" <<std::endl;
      break;
    default:
      std::cout<< "invalid options" <<std::endl;
      break;    
    }
  }
  
  //Get targets
  std::cout<<"Create TargetGenerator"<<std::endl;
  Arc::TargetGenerator test(mcfg);
  std::cout<<"Get Targets"<<std::endl;
  test.GetTargets();
  
}//end main
