#include "TargetRetrieverARC0.h"
#include <arc/misc/ClientInterface.h>
#include <arc/loader/Loader.h>
#include <arc/data/DataHandle.h>
#include <arc/data/DataBufferPar.h>
#include <arc/XMLNode.h>
#include <arc/URL.h>

namespace Arc{

  TargetRetrieverARC0::TargetRetrieverARC0(Arc::Config *cfg) 
    : Arc::TargetRetriever(cfg){}
  
  TargetRetrieverARC0::~TargetRetrieverARC0(){}

  ACC* TargetRetrieverARC0::Instance(Arc::Config *cfg, Arc::ChainContext*){
    return new TargetRetrieverARC0(cfg);
  }
  
  std::list<ACC*> TargetRetrieverARC0::getTargets(){
    
    std::list<ACC*> results;
    
    NS ns;
    Arc::Config cfg(ns);
    DMCConfig dmccfg;
    dmccfg.MakeConfig(cfg);
    Loader m_loader(&cfg);
    DataHandle handler(m_url);
    DataBufferPar buffer;
    handler->StartReading(buffer);

    int handle;
    unsigned int length;
    unsigned long long int offset;
    std::string result;

    while(!buffer.eof_read()){
      buffer.for_write(handle, length, offset, true);
      result.replace(offset, length, buffer[handle]);
      buffer.is_written(handle);
    }
    
    XMLNode XMLresult(result);

    XMLresult.SaveToStream(std::cout);

    //Next read XML result and decode into targets to be returned

    handler->StopReading();

    return results;

  }

} //namespace

acc_descriptors ARC_ACC_LOADER = {
  { "TargetRetrieverARC0", 0, &Arc::TargetRetrieverARC0::Instance },
  { NULL, 0, NULL }
};


