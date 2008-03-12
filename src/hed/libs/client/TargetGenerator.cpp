#include "TargetGenerator.h"
#include <arc/loader/Loader.h>
#include <arc/misc/ClientInterface.h>
#include <arc/XMLNode.h>
#include <arc/ArcConfig.h>

#include <iostream>

namespace Arc {

  TargetGenerator::TargetGenerator(Arc::Config &cfg){
    //Prepare Loader
    /*
    ACCConfig acccfg;
    NS ns;
    Config mcfg(ns);
    mcfg.SaveToStream(std::cout);
    XMLNode icfg = acccfg.MakeConfig(mcfg);
    mcfg.SaveToStream(std::cout);    
    //Lines below belong somewhere else
    XMLNode AnotherOne = icfg.NewChild("Component");
    AnotherOne.NewAttribute("name") = "TargetRetrieverARC0";
    AnotherOne.NewAttribute("id") = "retriever1";
    AnotherOne.NewChild("URL") = "www.tsl.uu.se";
  
    mcfg.SaveToStream(std::cout);
    */

    //Get those damn retrievers
    ACCloader = new Loader(&cfg);

  }
  
  TargetGenerator::~TargetGenerator(){
    //Should clean the mess we made
    std::cout << "TargetGenerator destructor" << std::endl;
    if (ACCloader) delete ACCloader;
  }
  
  
  std::list<ACC*> TargetGenerator::getTargets(){

    std::list<ACC*> result;

    //Loop over retrievers
    //This should be done using threads
    //ACC* something = ACCloader->getACC("retriever1");
    TargetRetriever* TR1 = dynamic_cast <TargetRetriever*> (ACCloader->getACC("retriever1"));
    TR1->getTargets();
    
    return result;

  }
    
} // namespace Arc
