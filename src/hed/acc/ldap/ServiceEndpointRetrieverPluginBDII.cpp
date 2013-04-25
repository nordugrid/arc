// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/Logger.h>
#include <arc/StringConv.h>
#include <arc/URL.h>
#include <arc/XMLNode.h>
#include <arc/data/DataBuffer.h>
#include <arc/data/DataHandle.h>

#include "ServiceEndpointRetrieverPluginBDII.h"

namespace Arc {

  Logger ServiceEndpointRetrieverPluginBDII::logger(Logger::getRootLogger(), "ServiceEndpointRetrieverPlugin.BDII");

  bool ServiceEndpointRetrieverPluginBDII::isEndpointNotSupported(const Endpoint& endpoint) const {
    const std::string::size_type pos = endpoint.URLString.find("://");
    return pos != std::string::npos && lower(endpoint.URLString.substr(0, pos)) != "ldap";
  }

  static URL CreateURL(std::string service) {
    std::string::size_type pos1 = service.find("://");
    if (pos1 == std::string::npos) {
      service = "ldap://" + service;
      pos1 = 4;
    } else if(lower(service.substr(0,pos1)) != "ldap") return URL();
    
    std::string::size_type pos2 = service.find(":", pos1 + 3);
    std::string::size_type pos3 = service.find("/", pos1 + 3);
    if (pos2 == std::string::npos && pos3 == std::string::npos) {
      service.append(":2170");
    }
    else if (pos2 == std::string::npos || pos2 > pos3) {
      service.insert(pos3, ":2170");
    }

    return service;
  }

  static std::string CreateBDIIResourceURL(const std::string& host) { return "ldap://" + host + ":2170/Mds-Vo-name=resource,o=grid"; }
  
  EndpointQueryingStatus ServiceEndpointRetrieverPluginBDII::Query(const UserConfig& uc,
                                                                    const Endpoint& rEndpoint,
                                                                    std::list<Endpoint>& seList,
                                                                    const EndpointQueryOptions<Endpoint>&) const {
    URL url(CreateURL(rEndpoint.URLString));
    url.ChangeLDAPScope(URL::subtree);
    //url.ChangeLDAPFilter("(|(GlueServiceType=bdii_site)(GlueServiceType=bdii_top))");
    if (!url) return EndpointQueryingStatus::FAILED;
    
    DataHandle handler(url, uc);
    DataBuffer buffer;
  
    if (!handler) {
      logger.msg(INFO, "Can't create information handle - is the ARC ldap DMC plugin available?");
      return EndpointQueryingStatus::FAILED;
    }
  
    if (!handler->StartReading(buffer)) return EndpointQueryingStatus::FAILED;
  
    int handle;
    unsigned int length;
    unsigned long long int offset;
    std::string result;
  
    while (buffer.for_write() || !buffer.eof_read())
      if (buffer.for_write(handle, length, offset, true)) {
        result.append(buffer[handle], length);
        buffer.is_written(handle);
      }
  
    if (!handler->StopReading()) return EndpointQueryingStatus::FAILED;
  
    XMLNode xmlresult(result);

    bool noServicesFound = true;
    XMLNodeList mdsVoNames = xmlresult.Path("o/Mds-Vo-name");
    for (std::list<XMLNode>::const_iterator itMds = mdsVoNames.begin(); itMds != mdsVoNames.end(); ++itMds) {
      for (XMLNode mdsVoNameSub = itMds->Get("Mds-Vo-name"); mdsVoNameSub; ++mdsVoNameSub) {
        for (XMLNode service = mdsVoNameSub["GlueServiceUniqueID"]; service; ++service) {
          if ((std::string)service["GlueServiceStatus"] != "OK") continue;
          
          const std::string serviceType = lower((std::string)service["GlueServiceType"]);
          Endpoint se;
          se.URLString = (std::string)service["GlueServiceEndpoint"];
          se.HealthState = "ok";
          
          if (serviceType == "bdii_top") {
            se.Capability.insert("information.discovery.registry");
            se.InterfaceName = "org.nordugrid.bdii";
          }
          else if (serviceType == "bdii_site") {
            se.Capability.insert("information.discovery.registry");
            se.InterfaceName = "org.nordugrid.bdii";
          }
          else if (serviceType == "org.glite.ce.cream") {
            logger.msg(INFO, "Adding CREAM computing service");
            se.InterfaceName = "org.glite.ce.cream";
            se.Capability.insert("information.lookup.job");
            se.Capability.insert("executionmanagement.jobcreation");
            se.Capability.insert("executionmanagement.jobdescription");
            se.Capability.insert("executionmanagement.jobmanager");
            
            // For CREAM also add resource BDII.
            Endpoint seInfo;
            seInfo.URLString = CreateBDIIResourceURL(URL(se.URLString).Host());
            seInfo.InterfaceName = "org.nordugrid.ldapglue1";
            seInfo.Capability.insert("information.discovery.resource");
            seList.push_back(seInfo);
          }
          else {
            // TODO: Handle other endpoints.
            continue;
          }

          noServicesFound = false;
          seList.push_back(se);
        }
      }
    }

    return noServicesFound ? EndpointQueryingStatus::NOINFORETURNED : EndpointQueryingStatus::SUCCESSFUL;
  }

} // namespace Arc
