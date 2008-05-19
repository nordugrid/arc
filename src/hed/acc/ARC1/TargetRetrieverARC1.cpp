#include "TargetRetrieverARC1.h"
#include <arc/client/ExecutionTarget.h>
#include <arc/data/DataHandle.h>
#include <arc/data/DataBufferPar.h>
#include <arc/StringConv.h>
#include <arc/XMLNode.h>
#include <arc/URL.h>

namespace Arc{

  TargetRetrieverARC1::TargetRetrieverARC1(Arc::Config *cfg) 
    : Arc::TargetRetriever(cfg){}
  
  TargetRetrieverARC1::~TargetRetrieverARC1(){}

  ACC* TargetRetrieverARC1::Instance(Arc::Config *cfg, Arc::ChainContext*){
    return new TargetRetrieverARC1(cfg);
  }
  
  /**
   * The present GetTargets implementation will
   * perform a "discover all" search based on the starting point url
   */
  void TargetRetrieverARC1::GetTargets(TargetGenerator &Mom, int TargetType, int DetailLevel){  
    
    //If TargetRetriever for this URL already exist, return
    if(Mom.DoIAlreadyExist(m_url)){
      return;
    }
    
    if(ServiceType=="computing") {
      //Add Service to TG list
      bool AddedService(Mom.AddService(m_url));
      
      //If added, interrogate service
      //Lines below this point depend on the usage of TargetGenerator
      //i.e. if it is used to find Targets for execution or storage,
      //and/or if the entire information is requested or only endpoints
      if(AddedService){
	InterrogateTarget(Mom, m_url, TargetType, DetailLevel);
      }
    }else if(ServiceType=="storage"){
    
    }else if(ServiceType=="index"){
     
    // TODO: ISIS

    }else{

      std::cout << "TargetRetrieverARC1 initialized with unknown url type" <<std::endl;

    }

  } //end GetTargets()

  void TargetRetrieverARC1::InterrogateTarget(TargetGenerator &Mom, std::string url, int TargetType, int DetailLevel){  
    
    // TODO: A-REX
    
  }//end TargetInterrogator
  
  std::list<std::string> TargetRetrieverARC1::getAttribute(std::string attr, Arc::XMLNode& node){
    std::list<std::string> results;
    
    XMLNodeList nodelist = node.XPathLookup(attr, NS());
    
    XMLNodeList::iterator iter;
    
    for(iter = nodelist.begin(); iter!= nodelist.end(); iter++){
      
      results.push_back((*iter));
      
    }

    return results;
    
  } //end getAttribute  

  
} //namespace
