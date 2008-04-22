#include "TargetGenerator.h"
#include "TargetRetriever.h"
#include <arc/loader/Loader.h>
#include <arc/misc/ClientInterface.h>
#include <arc/XMLNode.h>
#include <arc/ArcConfig.h>

#include <iostream>

namespace Arc {

  TargetGenerator::TargetGenerator(Arc::Config &cfg){

    ACCloader = new Loader(&cfg);

  }
  
  TargetGenerator::~TargetGenerator(){

    if (ACCloader) delete ACCloader;
  }
  
  
  void TargetGenerator::GetTargets(){

    //Get retrievers
    TargetRetriever* TR1 = dynamic_cast <TargetRetriever*> (ACCloader->getACC("retriever1"));

    //Get those targets ...
    //For now hardcoded test to get ExecutionTargets with full detail level
    //TargetType: Execution = 0, Storage = 1, ...
    //DetailLevel: Minimum = 0, Processed = 1, Full = 2
    TR1->GetTargets(*this, 0, 1);
    std::cout<<"Number of services found: " << FoundServices.size() << std::endl;
    std::cout<<"Number of Targets found: " << FoundTargets.size() << std::endl;

  }
    
  bool TargetGenerator::AddService(Arc::URL NewService){
    bool added = false;
    //lock this function call
    Glib::Mutex::Lock ServiceWriteLock(ServiceMutex);
    if(std::find(FoundServices.begin(), FoundServices.end(), NewService) == FoundServices.end()){
      FoundServices.push_back(NewService);
      added = true;
    }
    return added;
  }

  void TargetGenerator::AddTarget(Arc::ExecutionTarget NewTarget){
    //lock this function call
    Glib::Mutex::Lock TargetWriteLock(TargetMutex);
      FoundTargets.push_back(NewTarget);
  }

  bool TargetGenerator::DoIAlreadyExist(Arc::URL NewServer){
    //lock this function call
    Glib::Mutex::Lock ServerAddLock(ServerMutex);
    bool existence = true;

    //if not already found add to list of giises
    if(std::find(CheckedInfoServers.begin(), CheckedInfoServers.end(), NewServer) == CheckedInfoServers.end()){
      CheckedInfoServers.push_back(NewServer);
      existence = false;
    }
    return existence;
  }

} // namespace Arc
