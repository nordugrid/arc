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

    while(buffer.for_write() || !buffer.eof_read()) {
      buffer.for_write(handle, length, offset, true);
      result.replace(offset, length, buffer[handle]);
      buffer.is_written(handle);
    }
    
    handler->StopReading();

    XMLNode XMLresult(result);

    XMLresult.SaveToStream(std::cout);

    //Next read XML result and decode into targets to be returned


    //First do GIISes
    XMLNodeList GIISes = XMLresult.XPathLookup("//Mds-Vo-name", NS());
    std::cout << "#GIIS = " << GIISes.size() <<std::endl;

    XMLNodeList::iterator iter;
    for(iter = GIISes.begin(); iter!= GIISes.end(); iter++){
      if(!(*iter)["Mds-Service-type"]) continue; //remove first entry
      if((std::string)(*iter)["Mds-Reg-status"] == "PURGED" ) continue;      
      std::string url;
      url = (std::string) (*iter)["Mds-Service-type"] + "://" + 
	    (std::string) (*iter)["Mds-Service-hn"] + ":" +
            (std::string) (*iter)["Mds-Service-port"] + "/" +
	    (std::string) (*iter)["Mds-Service-Ldap-suffix"] + "?giisregistrationstatus?base";
      
      //      iter->SaveToStream(std::cout);
      std::cout << url << std::endl;
      
      NS ns;
      Arc::Config cfg(ns);
      cfg.NewChild("URL") = url;
      
      TargetRetrieverARC0 GIIS1(&cfg);
      
      std::list<ACC*> results2 = GIIS1.getTargets();

      results.insert(results.end(), results2.begin(), results2.end());

    }

    return results;

  }

} //namespace

acc_descriptors ARC_ACC_LOADER = {
  { "TargetRetrieverARC0", 0, &Arc::TargetRetrieverARC0::Instance },
  { NULL, 0, NULL }
};
