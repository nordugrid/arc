#include "TargetRetrieverARC0.h"
#include <arc/client/ExecutionTarget.h>
#include <arc/misc/ClientInterface.h>
#include <arc/loader/Loader.h>
#include <arc/data/DataHandle.h>
#include <arc/data/DataBufferPar.h>
#include <arc/StringConv.h>
#include <arc/XMLNode.h>
#include <arc/URL.h>

namespace Arc{

  TargetRetrieverARC0::TargetRetrieverARC0(Arc::Config *cfg) 
    : Arc::TargetRetriever(cfg){}
  
  TargetRetrieverARC0::~TargetRetrieverARC0(){}

  ACC* TargetRetrieverARC0::Instance(Arc::Config *cfg, Arc::ChainContext*){
    return new TargetRetrieverARC0(cfg);
  }
  
  /**
   * The present GetTargets implementation will
   * perform a "discover all" search based on the GIIS starting
   * points received either from config file of command line.
   */
  void TargetRetrieverARC0::GetTargets(TargetGenerator &Mom, int TargetType, int DetailLevel){  
    std::cout <<"TargetRetriverARC0 GetTargets called, URL = " << m_url << std::endl;
    
    //If TargetRetriever for this GIIS already exist, return
    if(Mom.DoIAlreadyExist(m_url)){
      return;
    }

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
      if(buffer.for_write(handle, length, offset, true)){
	result.append(buffer[handle], length);
	buffer.is_written(handle);
      }
    }
    
    handler->StopReading();

    XMLNode XMLresult(result);

    XMLresult.SaveToStream(std::cout);

    //Next read XML result and decode into further servers (GIIS) or services (GRIS)

    //First do GIISes (if any)

    XMLNodeList GIISes = XMLresult.XPathLookup("//Mds-Vo-name", NS());
    std::cout << "#GIIS = " << GIISes.size() <<std::endl;

    XMLNodeList::iterator iter;

    for(iter = GIISes.begin(); iter!= GIISes.end(); iter++){
      if(!(*iter)["Mds-Service-type"]) continue; //remove first entry
      if((std::string)(*iter)["Mds-Reg-status"] == "PURGED" ) continue;
      std::cout<<"This GIIS was accepted"<<std::endl;
      iter->SaveToStream(std::cout);
      std::string url;
      url = (std::string) (*iter)["Mds-Service-type"] + "://" + 
	    (std::string) (*iter)["Mds-Service-hn"] + ":" +
            (std::string) (*iter)["Mds-Service-port"] + "/" +
	    (std::string) (*iter)["Mds-Service-Ldap-suffix"] + "?giisregistrationstatus?base";

      std::cout << url << std::endl;
      
      NS ns;
      Arc::Config cfg(ns);
      cfg.NewChild("URL") = url;
      
      TargetRetrieverARC0 thisGIIS(&cfg);
      
      thisGIIS.GetTargets(Mom, TargetType, DetailLevel); 

    } //end GIISes

    //Next GRISes (if any)
    XMLNodeList GRISes = XMLresult.XPathLookup("//nordugrid-cluster-name", NS());
    std::cout << "#GRIS = " << GRISes.size() <<std::endl;
    for(iter = GRISes.begin(); iter!= GRISes.end(); iter++){
      if((std::string)(*iter)["Mds-Reg-status"] == "PURGED" ) continue;      
      
      std::string url;
      url = (std::string) (*iter)["Mds-Service-type"] + "://" + 
	    (std::string) (*iter)["Mds-Service-hn"] + ":" +
	    (std::string) (*iter)["Mds-Service-port"] + "/" +
	    (std::string) (*iter)["Mds-Service-Ldap-suffix"] + 
	    "??" + //attributes (empty means all)
	    "sub?" + // scope
	    "(|(objectclass=nordugrid-cluster)(objectclass=nordugrid-queue))"; //filter
      
      std::cout << "This is a GRIS:" << std::endl;
      std::cout << url << std::endl;

      //Should filter here on allowed VOs, not yet implemented
      
      //Add Service to TG list
      bool AddedService(Mom.AddService(url));

      //If added, interrogate service
      //Lines below this point depend on the usage of TargetGenerator
      //i.e. if it is used to find Targets for execution or storage,
      //and/or if the entire information is requested or only endpoints
      if(AddedService && TargetType == 0 && DetailLevel == 1){
	InterrogateTarget(Mom, url);
      }
    } //end GRISes

  } //end DiscoverExecutionServices()

  void TargetRetrieverARC0::InterrogateTarget(TargetGenerator &Mom, Arc::URL url){  
    std::cout <<"TargetRetriverARC0 TargetInterrogater called, URL = " << m_url << std::endl;
    
    //Query GRIS for all relevant information
    DataHandle handler(url);
    DataBufferPar buffer;
    handler->StartReading(buffer);
    
    int handle;
    unsigned int length;
    unsigned long long int offset;
    std::string result;
    
    while(buffer.for_write() || !buffer.eof_read()){
      if(buffer.for_write(handle, length, offset, true)){
	result.append(buffer[handle], length);
	buffer.is_written(handle);
      }
    }
 
    handler->StopReading();
    
    std::cout << "Interrogated GRIS, result = "<<result <<std::endl;
    
    XMLNode XMLresult(result);
    
    //Process information and prepare ExecutionTargets
    //Map 1 queue == 1 ExecutionTarget
    std::list<XMLNode> XMLqueues = XMLresult.XPathLookup("//nordugrid-queue-name[objectClass='nordugrid-queue']", NS());
    std::list<XMLNode>::iterator QueueIter;
    std::cout << "Number of queues found: " << XMLqueues.size() << std::endl;
    for(QueueIter = XMLqueues.begin(); QueueIter != XMLqueues.end(); QueueIter++){
      std::list<std::string> attributes;
      Arc::ExecutionTarget ThisTarget;

      //Find and fill Location information
      //Information like address, longitude, latitude etc not available in ARC0
      attributes = getAttribute("//nordugrid-cluster-name", XMLresult);
      if(attributes.size()){ThisTarget.Location.Name = *attributes.begin();}
      attributes = getAttribute("//nordugrid-cluster-owner", XMLresult);
      if(attributes.size()){ThisTarget.Location.Owner = *attributes.begin();}
      attributes = getAttribute("//nordugrid-cluster-location", XMLresult);
      if(attributes.size()){ThisTarget.Location.Owner = *attributes.begin();}


      //Find and fill Endpoint information (once again some entities are missing)
      attributes = getAttribute("//nordugrid-cluster-contactstring", XMLresult);
      if(attributes.size()){ThisTarget.Endpoint.URL = *attributes.begin();}
      ThisTarget.Endpoint.InterfaceName = "GridFTP";
      ThisTarget.Endpoint.Implementor = "NorduGrid";      
      ThisTarget.Endpoint.ImplementationName = "ARC0";      
      attributes = getAttribute("//nordugrid-cluster-middleware", XMLresult);
      if(attributes.size()){ThisTarget.Endpoint.ImplementationVersion = *attributes.begin();}
      attributes = getAttribute("//nordugrid-queue-status", XMLresult);
      if(attributes.size()){ThisTarget.Endpoint.HealthState = *attributes.begin();}
      attributes = getAttribute("//nordugrid-cluster-issuerca", XMLresult);
      if(attributes.size()){ThisTarget.Endpoint.IssuerCA = *attributes.begin();}
      attributes = getAttribute("//nordugrid-cluster-nodeaccess", XMLresult);
      if(attributes.size()){ThisTarget.Endpoint.Staging = *attributes.begin();}

      //Jobs, RTEs, Memory, CPU ...
      attributes = getAttribute("//nordugrid-queue-name", XMLresult);
      if(attributes.size()){ThisTarget.MappingQueue = *attributes.begin();}
      attributes = getAttribute("//nordugrid-queue-maxwalltime", XMLresult);
      if(attributes.size()){ThisTarget.MaxWallTime = stringtoi(*attributes.begin());}
      attributes = getAttribute("//nordugrid-queue-minwalltime", XMLresult);
      if(attributes.size()){ThisTarget.MinWallTime = stringtoi(*attributes.begin());}
      attributes = getAttribute("//nordugrid-queue-defaultwalltime", XMLresult);
      if(attributes.size()){ThisTarget.DefaultWallTime = stringtoi(*attributes.begin());}
      attributes = getAttribute("//nordugrid-queue-maxcputime", XMLresult);
      if(attributes.size()){ThisTarget.MaxCPUTime = stringtoi(*attributes.begin());}
      attributes = getAttribute("//nordugrid-queue-mincputime", XMLresult);
      if(attributes.size()){ThisTarget.MinCPUTime = stringtoi(*attributes.begin());}
      attributes = getAttribute("//nordugrid-queue-defaultcputime", XMLresult);
      if(attributes.size()){ThisTarget.DefaultCPUTime = stringtoi(*attributes.begin());}
      attributes = getAttribute("//nordugrid-queue-defaultcputime", XMLresult);
      if(attributes.size()){ThisTarget.DefaultCPUTime = stringtoi(*attributes.begin());}
      attributes = getAttribute("//nordugrid-queue-maxrunning", XMLresult);
      if(attributes.size()){ThisTarget.MaxRunningJobs = stringtoi(*attributes.begin());}
      attributes = getAttribute("//nordugrid-queue-maxqueable", XMLresult);
      if(attributes.size()){ThisTarget.MaxWaitingJobs = stringtoi(*attributes.begin());}
      attributes = getAttribute("//nordugrid-queue-maxuserrun", XMLresult);
      if(attributes.size()){ThisTarget.MaxUserRunningJobs = stringtoi(*attributes.begin());}
      attributes = getAttribute("//nordugrid-queue-schedulingpolicy", XMLresult);
      if(attributes.size()){ThisTarget.SchedulingPolicy = *attributes.begin();}
      attributes = getAttribute("//nordugrid-queue-nodememory", XMLresult);
      if(attributes.size()){ThisTarget.NodeMemory = stringtoi(*attributes.begin());
      } else {
	attributes = getAttribute("//nordugrid-cluster-nodememory", XMLresult);
	if(attributes.size()){ThisTarget.NodeMemory = stringtoi(*attributes.begin());}
      }
      attributes = getAttribute("//nordugrid-cluster-sessiondir-free", XMLresult);
      if(attributes.size()){ThisTarget.MaxDiskSpace = stringtoi(*attributes.begin());}
      attributes = getAttribute("//nordugrid-cluster-localse", XMLresult);
      if(attributes.size()){ThisTarget.DefaultStorageService = *attributes.begin();}
      attributes = getAttribute("//nordugrid-queue-running", XMLresult);
      if(attributes.size()){ThisTarget.RunningJobs = stringtoi(*attributes.begin());}
      attributes = getAttribute("//nordugrid-queue-queued", XMLresult);
      if(attributes.size()){ThisTarget.WaitingJobs = stringtoi(*attributes.begin());}
      attributes = getAttribute("//nordugrid-queue-prelrmsqueued", XMLresult);
      if(attributes.size()){ThisTarget.PreLRMSWaitingJobs = stringtoi(*attributes.begin());}
      
      //Register target in TargetGenerator list
      Mom.AddTarget(ThisTarget);

    } // end loop over queues
    
  }//end TargetInterrogator
  
  std::list<std::string> TargetRetrieverARC0::getAttribute(std::string attr, Arc::XMLNode& node){
    std::list<std::string> results;
    
    XMLNodeList nodelist = node.XPathLookup(attr, NS());
    
    XMLNodeList::iterator iter;
    
    for(iter = nodelist.begin(); iter!= nodelist.end(); iter++){
      
      results.push_back((*iter));
      
    }

    return results;
    
  } //end getAttribute

  

  
} //namespace

acc_descriptors ARC_ACC_LOADER = {
  { "TargetRetrieverARC0", 0, &Arc::TargetRetrieverARC0::Instance },
  { NULL, 0, NULL }
};
