#include "TargetRetrieverARC0.h"
#include <arc/client/ExecutionTarget.h>
#include <arc/data/DataHandle.h>
#include <arc/data/DataBufferPar.h>
#include <arc/StringConv.h>
#include <arc/XMLNode.h>
#include <arc/URL.h>

namespace Arc {

  TargetRetrieverARC0::TargetRetrieverARC0(Config *cfg)
    : TargetRetriever(cfg) {}

  TargetRetrieverARC0::~TargetRetrieverARC0() {}

  ACC *TargetRetrieverARC0::Instance(Config *cfg, ChainContext *) {
    return new TargetRetrieverARC0(cfg);
  }

  /**
   * The present GetTargets implementation will
   * perform a "discover all" search based on the starting point url
   */
  void TargetRetrieverARC0::GetTargets(TargetGenerator& Mom, int TargetType,
				       int DetailLevel) {

    //If TargetRetriever for this URL already exist, return
    if (Mom.DoIAlreadyExist(m_url))
      return;

    if (ServiceType == "computing") {
      //Add Service to TG list
      bool AddedService(Mom.AddService(m_url));

      //If added, interrogate service
      //Lines below this point depend on the usage of TargetGenerator
      //i.e. if it is used to find Targets for execution or storage,
      //and/or if the entire information is requested or only endpoints
      if (AddedService)
	InterrogateTarget(Mom, m_url, TargetType, DetailLevel);
    }
    else if (ServiceType == "storage") {}
    else if (ServiceType == "index") {

      DataHandle handler(m_url + "?giisregistrationstatus?base");
      DataBufferPar buffer;
      if (!handler->StartReading(buffer))
	return;

      int handle;
      unsigned int length;
      unsigned long long int offset;
      std::string result;

      while (buffer.for_write() || !buffer.eof_read())
	if (buffer.for_write(handle, length, offset, true)) {
	  result.append(buffer[handle], length);
	  buffer.is_written(handle);
	}

      if (!handler->StopReading())
	return;

      XMLNode XMLresult(result);

      //Next read XML result and decode into further servers (GIIS) or services (GRIS)

      //First do GIISes (if any)
      XMLNodeList GIISes = XMLresult.XPathLookup("//Mds-Vo-name", NS());

      XMLNodeList::iterator iter;

      /*
         for(iter = GIISes.begin(); iter!= GIISes.end(); iter++){
         if(!(*iter)["Mds-Service-type"]) continue; //remove first entry
         if((std::string)(*iter)["Mds-Reg-status"] == "PURGED" ) continue;
         std::cout<<"This GIIS was accepted"<<std::endl;
         iter->SaveToStream(std::cout);
         std::string url;
         url = (std::string) (*iter)["Mds-Service-type"] + "://" +
          (std::string) (*iter)["Mds-Service-hn"] + ":" +
          (std::string) (*iter)["Mds-Service-port"] + "/" +
          (std::string) (*iter)["Mds-Service-Ldap-suffix"];

         NS ns;
         Config cfg(ns);
         XMLNode URLXML = cfg.NewChild("URL") = url;
         URLXML.NewAttribute("ServiceType") = "index";

         TargetRetrieverARC0 thisGIIS(&cfg);

         thisGIIS.GetTargets(Mom, TargetType, DetailLevel);

         } //end GIISes
       */

      //Next GRISes (if any)
      XMLNodeList GRISes = XMLresult.XPathLookup("//nordugrid-cluster-name[objectClass='MdsService']", NS());
      for (iter = GRISes.begin(); iter != GRISes.end(); iter++) {
	if ((std::string)(*iter)["Mds-Reg-status"] == "PURGED")
	  continue;

	XMLNode ThisGRIS = (XMLNode)(*iter);

	std::string url;
	url = (std::string)(*iter)["Mds-Service-type"] + "://" +
	      (std::string)(*iter)["Mds-Service-hn"] + ":" +
	      (std::string)(*iter)["Mds-Service-port"] + "/" +
	      (std::string)(*iter)["Mds-Service-Ldap-suffix"];

	//Add Service to TG list
	bool AddedService(Mom.AddService(url));

	std::cout << "This is a GRIS: " << url << std::endl;

	//If added, interrogate service
	//Lines below this point depend on the usage of TargetGenerator
	//i.e. if it is used to find Targets for execution or storage,
	//and/or if the entire information is requested or only endpoints
	if (AddedService)
	  InterrogateTarget(Mom, url, TargetType, DetailLevel);
      } //end GRISes

      //end if this TR was initialized with a GIIS url i.e. index
    }
    else

      std::cout << "TargetRetrieverARC0 initialized with unknown url type" << std::endl;


  } //end GetTargets()

  void TargetRetrieverARC0::InterrogateTarget(TargetGenerator& Mom,
					      std::string url, int TargetType,
					      int DetailLevel) {

    //Query GRIS for all relevant information
    DataHandle handler(url + "??sub?(|(objectclass=nordugrid-cluster)(objectclass=nordugrid-queue))");
    DataBufferPar buffer;

    if (!handler->StartReading(buffer))
      return;

    int handle;
    unsigned int length;
    unsigned long long int offset;
    std::string result;

    while (buffer.for_write() || !buffer.eof_read())
      if (buffer.for_write(handle, length, offset, true)) {
	result.append(buffer[handle], length);
	buffer.is_written(handle);
      }

    if (!handler->StopReading())
      return;

    XMLNode XMLresult(result);

    //Process information and prepare ExecutionTargets
    //Map 1 queue == 1 ExecutionTarget
    std::list<XMLNode> XMLqueues = XMLresult.XPathLookup("//nordugrid-queue-name[objectClass='nordugrid-queue']", NS());
    std::list<XMLNode>::iterator QueueIter;

    for (QueueIter = XMLqueues.begin(); QueueIter != XMLqueues.end(); QueueIter++) {
      std::list<std::string> attributes;
      ExecutionTarget ThisTarget;

      ThisTarget.GridFlavour = "ARC0";
      ThisTarget.Source = url;

      //Find and fill location information
      //Information like address, longitude, latitude etc not available in ARC0
      attributes = getAttribute("//nordugrid-cluster-name", XMLresult);
      if (attributes.size())
	ThisTarget.Name = *attributes.begin();
      attributes = getAttribute("//nordugrid-cluster-aliasname", XMLresult);
      if (attributes.size())
	ThisTarget.Alias = *attributes.begin();
      attributes = getAttribute("//nordugrid-cluster-owner", XMLresult);
      if (attributes.size())
	ThisTarget.Owner = *attributes.begin();
      attributes = getAttribute("//nordugrid-cluster-location", XMLresult);
      if (attributes.size())
	ThisTarget.PostCode = *attributes.begin();
      //Find and fill endpoint information (once again some entities are missing)
      attributes = getAttribute("//nordugrid-cluster-contactstring", XMLresult);
      if (attributes.size())
	ThisTarget.url = *attributes.begin();
      ThisTarget.InterfaceName = "GridFTP";
      ThisTarget.Implementor = "NorduGrid";
      ThisTarget.ImplementationName = "ARC0";
      attributes = getAttribute("//nordugrid-cluster-middleware", XMLresult);
      if (attributes.size())
	ThisTarget.ImplementationVersion = *attributes.begin();
      attributes = getAttribute("//nordugrid-queue-status", XMLresult);
      if (attributes.size())
	ThisTarget.HealthState = *attributes.begin();
      attributes = getAttribute("//nordugrid-cluster-issuerca", XMLresult);
      if (attributes.size())
	ThisTarget.IssuerCA = *attributes.begin();
      attributes = getAttribute("//nordugrid-cluster-nodeaccess", XMLresult);
      if (attributes.size())
	ThisTarget.Staging = *attributes.begin();
      //Jobs, RTEs, Memory, CPU ...
      attributes = getAttribute("//nordugrid-queue-name", XMLresult);
      if (attributes.size())
	ThisTarget.MappingQueue = *attributes.begin();
      attributes = getAttribute("//nordugrid-queue-maxwalltime", XMLresult);
      if (attributes.size())
	ThisTarget.MaxWallTime = stringtoi(*attributes.begin());
      attributes = getAttribute("//nordugrid-queue-minwalltime", XMLresult);
      if (attributes.size())
	ThisTarget.MinWallTime = stringtoi(*attributes.begin());
      attributes = getAttribute("//nordugrid-queue-defaultwalltime", XMLresult);
      if (attributes.size())
	ThisTarget.DefaultWallTime = stringtoi(*attributes.begin());
      attributes = getAttribute("//nordugrid-queue-maxcputime", XMLresult);
      if (attributes.size())
	ThisTarget.MaxCPUTime = stringtoi(*attributes.begin());
      attributes = getAttribute("//nordugrid-queue-mincputime", XMLresult);
      if (attributes.size())
	ThisTarget.MinCPUTime = stringtoi(*attributes.begin());
      attributes = getAttribute("//nordugrid-queue-defaultcputime", XMLresult);
      if (attributes.size())
	ThisTarget.DefaultCPUTime = stringtoi(*attributes.begin());
      attributes = getAttribute("//nordugrid-queue-defaultcputime", XMLresult);
      if (attributes.size())
	ThisTarget.DefaultCPUTime = stringtoi(*attributes.begin());
      attributes = getAttribute("//nordugrid-queue-maxrunning", XMLresult);
      if (attributes.size())
	ThisTarget.MaxRunningJobs = stringtoi(*attributes.begin());
      attributes = getAttribute("//nordugrid-queue-maxqueable", XMLresult);
      if (attributes.size())
	ThisTarget.MaxWaitingJobs = stringtoi(*attributes.begin());
      attributes = getAttribute("//nordugrid-queue-maxuserrun", XMLresult);
      if (attributes.size())
	ThisTarget.MaxUserRunningJobs = stringtoi(*attributes.begin());
      attributes = getAttribute("//nordugrid-queue-schedulingpolicy", XMLresult);
      if (attributes.size())
	ThisTarget.SchedulingPolicy = *attributes.begin();
      attributes = getAttribute("//nordugrid-queue-nodememory", XMLresult);
      if (attributes.size())
	ThisTarget.NodeMemory = stringtoi(*attributes.begin());
      else {
	attributes = getAttribute("//nordugrid-cluster-nodememory", XMLresult);
	if (attributes.size())
	  ThisTarget.NodeMemory = stringtoi(*attributes.begin());
      }
      attributes = getAttribute("//nordugrid-cluster-sessiondir-free", XMLresult);
      if (attributes.size())
	ThisTarget.MaxDiskSpace = stringtoi(*attributes.begin());
      attributes = getAttribute("//nordugrid-cluster-localse", XMLresult);
      if (attributes.size())
	ThisTarget.DefaultStorageService = *attributes.begin();
      attributes = getAttribute("//nordugrid-queue-running", XMLresult);
      if (attributes.size())
	ThisTarget.RunningJobs = stringtoi(*attributes.begin());
      attributes = getAttribute("//nordugrid-queue-queued", XMLresult);
      if (attributes.size())
	ThisTarget.WaitingJobs = stringtoi(*attributes.begin());
      attributes = getAttribute("//nordugrid-queue-prelrmsqueued", XMLresult);
      if (attributes.size())
	ThisTarget.PreLRMSWaitingJobs = stringtoi(*attributes.begin());
      //Register target in TargetGenerator list
      Mom.AddTarget(ThisTarget);

    } // end loop over queues

  } //end TargetInterrogator

  std::list<std::string> TargetRetrieverARC0::getAttribute(std::string attr,
							   XMLNode& node) {
    std::list<std::string> results;

    XMLNodeList nodelist = node.XPathLookup(attr, NS());

    XMLNodeList::iterator iter;

    for (iter = nodelist.begin(); iter != nodelist.end(); iter++)
      results.push_back((*iter));

    return results;

  } //end getAttribute

} //namespace
