#include "TargetRetrieverARC0.h"
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
  
  std::list<Glue2::ComputingService_t> TargetRetrieverARC0::getTargets(){
    
    std::cout <<"TargetRetriverARC0 getTargets called, URL = " << m_url << std::endl;
    
    std::list<Glue2::ComputingService_t> results;
    
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

    //XMLresult.SaveToStream(std::cout);

    //Next read XML result and decode into targets to be returned

    //First do GIISes (if any)
    XMLNodeList GIISes = XMLresult.XPathLookup("//Mds-Vo-name", NS());
    std::cout << "#GIIS = " << GIISes.size() <<std::endl;

    XMLNodeList::iterator iter;

    //Loop over GIISes commented out for now
    /*
    for(iter = GIISes.begin(); iter!= GIISes.end(); iter++){
      if(!(*iter)["Mds-Service-type"]) continue; //remove first entry
      if((std::string)(*iter)["Mds-Reg-status"] == "PURGED" ) continue;      
      std::string url;
      url = (std::string) (*iter)["Mds-Service-type"] + "://" + 
	    (std::string) (*iter)["Mds-Service-hn"] + ":" +
            (std::string) (*iter)["Mds-Service-port"] + "/" +
	    (std::string) (*iter)["Mds-Service-Ldap-suffix"] + "?giisregistrationstatus?base";
      
      //iter->SaveToStream(std::cout);
      //std::cout << url << std::endl;
      
      NS ns;
      Arc::Config cfg(ns);
      cfg.NewChild("URL") = url;
      
      TargetRetrieverARC0 GIIS1(&cfg);
      
      std::list<ACC*> results2 = GIIS1.getTargets();

      results.insert(results.end(), results2.begin(), results2.end());

    } //end GIISes
    */

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
      
      //Query GRIS for all relevant information
      handler = url;
      DataBufferPar buffer;
      handler->StartReading(buffer);
      std::string result;
      
      while(buffer.for_write() || !buffer.eof_read()){
	if(buffer.for_write(handle, length, offset, true)){
	  result.append(buffer[handle], length);
	  buffer.is_written(handle);
	}
      }

      handler->StopReading();

      std::cout << "result = "<<result <<std::endl; 
      
      XMLNode XMLresult(result);
      
      //Check how many cluster we got (Should only be one, but in the event of ... )
      std::list<std::string> attributes = getAttribute(
      "//nordugrid-cluster-name[objectClass='nordugrid-cluster']", XMLresult);
      std::cout << "How many nordugrid-cluster-name: " << attributes.size() << std::endl;

      for(int i = 0; i < attributes.size(); i++){ //in the event that one GRIS reports two or more clusters
	std::cout << "Processing cluster nr " << i << std::endl;
	//Returnobject to be added to the results
	Glue2::ComputingService_t ThisResult;
	ThisResult.ComputingEndpoint.push_back(Glue2::ComputingEndpoint_t());
	ThisResult.ComputingResource.push_back(Glue2::ComputingResource_t());
      
	//Read attributes, fill ThisResults according to chosen ARC0 - GLUE2 mapping
	//First do all attributes associated to nordugrid-cluster	
	
	if(attributes.size()){ThisResult.ID = *attributes.begin();}      
	if(attributes.size()){ThisResult.ComputingEndpoint.begin()->ID = *attributes.begin();}            
	if(attributes.size()){ThisResult.ComputingResource.begin()->ID = *attributes.begin();}      
	attributes = getAttribute("//nordugrid-cluster-contactstring", XMLresult);
	if(attributes.size()){ThisResult.ComputingEndpoint.begin()->URL = *attributes.begin();}
	attributes = getAttribute("//nordugrid-cluster-aliasname", XMLresult);
	if(attributes.size()){ThisResult.Name = *attributes.begin();}
	attributes = getAttribute("//nordugrid-cluster-support", XMLresult);
	if(attributes.size()){ThisResult.Contact.URL = *attributes.begin();}
	attributes = getAttribute("//nordugrid-cluster-lrms-type", XMLresult);
	if(attributes.size()){ThisResult.ComputingResource.begin()->LRMSType = *attributes.begin();}
	attributes = getAttribute("//nordugrid-cluster-lrms-version", XMLresult);
	if(attributes.size()){ThisResult.ComputingResource.begin()->LRMSVersion = *attributes.begin();}
	attributes = getAttribute("//nordugrid-cluster-lrms-config", XMLresult);
	if(attributes.size()){ThisResult.ComputingResource.begin()->LRMSOtherInfo = *attributes.begin();}
	//NOT YET MAPPED
	attributes = getAttribute("//nordugrid-cluster-architecture", XMLresult);
	attributes = getAttribute("//nordugrid-cluster-opsys", XMLresult);
	attributes = getAttribute("//nordugrid-cluster-homogeneity", XMLresult);
	attributes = getAttribute("//nordugrid-cluster-nodecpu", XMLresult);
	//
	attributes = getAttribute("//nordugrid-cluster-totalcpus", XMLresult);
	if(attributes.size()){ThisResult.ComputingResource.begin()->TotalLogicalCPUs = stringtoi(*attributes.begin());}
	attributes = getAttribute("//nordugrid-cluster-cpudistribution", XMLresult);
	if(attributes.size()){ThisResult.ComputingResource.begin()->LogicalCPUDistribution = *attributes.begin();}
	attributes = getAttribute("//nordugrid-cluster-sessiondir-free", XMLresult);
	if(attributes.size()){ThisResult.ComputingResource.begin()->GridAreaFree = stringtoi(*attributes.begin());}
	attributes = getAttribute("//nordugrid-cluster-sessiondir-total", XMLresult);
	if(attributes.size()){ThisResult.ComputingResource.begin()->GridAreaTotal = stringtoi(*attributes.begin());}
	attributes = getAttribute("//nordugrid-cluster-cache-free", XMLresult);
	if(attributes.size()){ThisResult.ComputingResource.begin()->CacheFree = stringtoi(*attributes.begin());}
	attributes = getAttribute("//nordugrid-cluster-cache-total", XMLresult);
	if(attributes.size()){ThisResult.ComputingResource.begin()->CacheTotal = stringtoi(*attributes.begin());}
	attributes = getAttribute("//nordugrid-cluster-runtimeenvironment", XMLresult);
	//Loop over all RTEs and add them as ApplicationEnvironments in ComputingResource
	std::list<std::string>::iterator attriter;
	for(attriter = attributes.begin();attriter != attributes.end(); attriter++){
	  ThisResult.ComputingResource.begin()->ApplicationEnvironments.push_back(Glue2::ApplicationEnvironment_t()); 
	  ThisResult.ComputingResource.begin()->ApplicationEnvironments.rbegin()->Name = (*attriter);
	}
	attributes = getAttribute("//nordugrid-cluster-localse", XMLresult); //Not yet mapped
	attributes = getAttribute("//nordugrid-cluster-middleware", XMLresult);
	if(attributes.size()){ThisResult.Type = *attributes.begin();}
	attributes = getAttribute("//nordugrid-cluster-totaljobs", XMLresult);
	if(attributes.size()){ThisResult.TotalJobs = stringtoi(*attributes.begin());}
	attributes = getAttribute("//nordugrid-cluster-queuedjobs", XMLresult);
	if(attributes.size()){ThisResult.WaitingJobs = stringtoi(*attributes.begin());}
	attributes = getAttribute("//nordugrid-cluster-location", XMLresult);
	if(attributes.size()){ThisResult.Location.PostCode = *attributes.begin();}
	attributes = getAttribute("//nordugrid-cluster-issuerca", XMLresult);
	if(attributes.size()){ThisResult.ComputingEndpoint.begin()->IssuerCA = *attributes.begin();}
	attributes = getAttribute("//nordugrid-cluster-nodeaccess", XMLresult); //Not yet mapped
	attributes = getAttribute("//nordugrid-cluster-comment", XMLresult);
	if(attributes.size()){ThisResult.OtherInfo.push_back(*attributes.begin());}
	attributes = getAttribute("//nordugrid-cluster-interactive-contactstring", XMLresult); //Not yet mapped	
	attributes = getAttribute("//nordugrid-cluster-benchmark", XMLresult);
	if(attributes.size()){ThisResult.ComputingResource.begin()->Benchmark = *attributes.begin();}
	attributes = getAttribute("//nordugrid-cluster-sessiondir-lifetime", XMLresult);
	if(attributes.size()){ThisResult.ComputingResource.begin()->GridAreaLifeTime = stringtoi(*attributes.begin());}
	attributes = getAttribute("//nordugrid-cluster-prelrmsqueued", XMLresult);
	if(attributes.size()){ThisResult.PreLRMSWaitingJobs = stringtoi(*attributes.begin());}
	attributes = getAttribute("//nordugrid-cluster-issuerca-hash", XMLresult); //Not yet mapped
	attributes = getAttribute("//nordugrid-cluster-trustedca", XMLresult); //Not yet mapped
	attributes = getAttribute("//nordugrid-cluster-acl", XMLresult); //Not yet mapped

	std::cout << "Did I get anything reasonable? nordugrid-cluster-aliasname = " << ThisResult.Name << std::endl;
	
	std::cout << "How many RTEs did I get:" <<  ThisResult.ComputingResource.begin()->ApplicationEnvironments.size()<< std::endl;
	results.push_back(ThisResult);

	//Next do the queues ...
	std::list<XMLNode> XMLqueues = XMLresult.XPathLookup("//nordugrid-queue-name[objectClass='nordugrid-queue']", NS());
	std::list<XMLNode>::iterator QueueIter;
	std::cout << "Number of queues found: " << XMLqueues.size() << std::endl;	
	for(QueueIter = XMLqueues.begin(); QueueIter != XMLqueues.end(); QueueIter++){
	  ThisResult.ComputingShare.push_back(Glue2::ComputingShare_t());
	  attributes = getAttribute("//nordugrid-queue-name[objectClass='nordugrid-queue']", (*QueueIter));
	  if(attributes.size()){ThisResult.ComputingShare.begin()->Name = *attributes.begin();}
	  attributes = getAttribute("//nordugrid-queue-status", (*QueueIter));
	  if(attributes.size()){ThisResult.ComputingShare.begin()->ServingState = *attributes.begin();}
	  attributes = getAttribute("//nordugrid-queue-running", (*QueueIter));
	  if(attributes.size()){ThisResult.ComputingShare.begin()->RunningJobs = stringtoi(*attributes.begin());}
	  attributes = getAttribute("//nordugrid-queue-queued", (*QueueIter));
	  if(attributes.size()){ThisResult.ComputingShare.begin()->WaitingJobs = stringtoi(*attributes.begin());}
	  attributes = getAttribute("//nordugrid-queue-maxrunning", (*QueueIter));
	  if(attributes.size()){ThisResult.ComputingShare.begin()->MaxRunningJobs = stringtoi(*attributes.begin());}
	  attributes = getAttribute("//nordugrid-queue-maxqueuable", (*QueueIter));
	  if(attributes.size()){ThisResult.ComputingShare.begin()->MaxWaitingJobs = stringtoi(*attributes.begin());}
	  attributes = getAttribute("//nordugrid-queue-maxuserrun", (*QueueIter));
	  if(attributes.size()){ThisResult.ComputingShare.begin()->MaxUserRunningJobs = stringtoi(*attributes.begin());}
	  attributes = getAttribute("//nordugrid-queue-maxcputime", (*QueueIter));
	  if(attributes.size()){ThisResult.ComputingShare.begin()->MaxCPUTime = stringtoi(*attributes.begin());}	  
	  attributes = getAttribute("//nordugrid-queue-mincputime", (*QueueIter));
	  if(attributes.size()){ThisResult.ComputingShare.begin()->MinCPUTime = stringtoi(*attributes.begin());}
	  attributes = getAttribute("//nordugrid-queue-defaultcputime", (*QueueIter));
	  if(attributes.size()){ThisResult.ComputingShare.begin()->DefaultCPUTime = stringtoi(*attributes.begin());}
	  attributes = getAttribute("//nordugrid-queue-schedulingpolicy", (*QueueIter));
	  if(attributes.size()){ThisResult.ComputingShare.begin()->SchedulingPolicy = *attributes.begin();}
	  bool EEcreated = false;
	  attributes = getAttribute("//nordugrid-queue-totalcpus", (*QueueIter));
	  if(attributes.size()){
	    ThisResult.ComputingResource.begin()->ExecutionEnvironment.push_back(Glue2::ExecutionEnvironment_t());  
	    ThisResult.ComputingResource.begin()->ExecutionEnvironment.rbegin()->PhysicalCPUs = stringtoi(*attributes.begin());
	    EEcreated = true;
	  }
	  attributes = getAttribute("//nordugrid-queue-nodememory", (*QueueIter));
	  if(attributes.size()){
	    if(!EEcreated){
	      ThisResult.ComputingResource.begin()->ExecutionEnvironment.push_back(Glue2::ExecutionEnvironment_t());  
	      EEcreated = true;
	    }
	    ThisResult.ComputingResource.begin()->ExecutionEnvironment.rbegin()->MainMemorySize = stringtoi(*attributes.begin());
	  }
	  attributes = getAttribute("//nordugrid-queue-opsys", (*QueueIter));
	  if(attributes.size()){
	    if(!EEcreated){
	      ThisResult.ComputingResource.begin()->ExecutionEnvironment.push_back(Glue2::ExecutionEnvironment_t());  
	      EEcreated = true;
	    }
	    ThisResult.ComputingResource.begin()->ExecutionEnvironment.rbegin()->OSName = *attributes.begin();
	  }

	} //end loop over clusters

      } //end loop over possible multiple cluster entries from GRIS
      
    } //end GRISes
    
    return results;
    
  } //end getTargets()
  
  
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
