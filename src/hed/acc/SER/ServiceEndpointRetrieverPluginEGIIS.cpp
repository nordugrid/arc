// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/Logger.h>
#include <arc/URL.h>
#include <arc/XMLNode.h>
#include <arc/data/DataBuffer.h>
#include <arc/data/DataHandle.h>

#include "ServiceEndpointRetrieverPluginEGIIS.h"

namespace Arc {

  Logger ServiceEndpointRetrieverPluginEGIIS::logger(Logger::getRootLogger(), "ServiceEndpointRetrieverPlugin.EGIIS");

/*
  static URL CreateURL(std::string service, ServiceType st) {
    std::string::size_type pos1 = service.find("://");
    if (pos1 == std::string::npos) {
      service = "ldap://" + service;
      pos1 = 4;
    } else {
      if(lower(service.substr(0,pos1)) != "ldap") return URL();
    }
    std::string::size_type pos2 = service.find(":", pos1 + 3);
    std::string::size_type pos3 = service.find("/", pos1 + 3);
    if (pos3 == std::string::npos) {
      if (pos2 == std::string::npos)
        service += ":2135";
      if (st == COMPUTING)
        service += "/Mds-Vo-name=local, o=Grid";
      else
        service += "/Mds-Vo-name=NorduGrid, o=Grid";
    }
    else if (pos2 == std::string::npos || pos2 > pos3)
      service.insert(pos3, ":2135");
    return service;
  }
*/

  EndpointQueryingStatus ServiceEndpointRetrieverPluginEGIIS::Query(const UserConfig& uc,
                                                                    const RegistryEndpoint& rEndpoint,
                                                                    std::list<ServiceEndpoint>& seList,
                                                                    const EndpointQueryOptions<ServiceEndpoint>&) const {
    EndpointQueryingStatus s(EndpointQueryingStatus::STARTED);

    URL url(rEndpoint.URLString);
    url.ChangeLDAPScope(URL::base);
    url.AddLDAPAttribute("giisregistrationstatus");
    if (!url) {
      s = EndpointQueryingStatus::FAILED;
      return s;
    }

    DataHandle handler(url, uc);
    DataBuffer buffer;

    if (!handler) {
      logger.msg(INFO, "Can't create information handle - is the ARC ldap DMC plugin available?");
      s = EndpointQueryingStatus::FAILED;
      return s;
    }

    if (!handler->StartReading(buffer)) {
      return s;
    }

    int handle;
    unsigned int length;
    unsigned long long int offset;
    std::string result;

    while (buffer.for_write() || !buffer.eof_read())
      if (buffer.for_write(handle, length, offset, true)) {
        result.append(buffer[handle], length);
        buffer.is_written(handle);
      }

    if (!handler->StopReading()) {
      s = EndpointQueryingStatus::FAILED;
      return s;
    }

    XMLNode xmlresult(result);
    XMLNodeList mdsVoNames = xmlresult.Path("o/Mds-Vo-name");
    for (XMLNodeList::iterator itMds = mdsVoNames.begin(); itMds != mdsVoNames.end(); ++itMds) {
      for (int i = 0; i < itMds->Size(); ++i) {
        if ((std::string)itMds->Child(i)["Mds-Reg-status"] == "PURGED") {
          continue;
        }
        ServiceEndpoint se((std::string)itMds->Child(i)["Mds-Service-type"] + "://" +
                           (std::string)itMds->Child(i)["Mds-Service-hn"] + ":" +
                           (std::string)itMds->Child(i)["Mds-Service-port"] + "/" +
                           (std::string)itMds->Child(i)["Mds-Service-Ldap-suffix"]);
        if (itMds->Child(i).Name() == "Mds-Vo-name") {
          se.Capability.push_back("information.discovery.registry");
          se.InterfaceName = supportedInterfaces.front();
        }
        else if (itMds->Child(i).Name() == "nordugrid-cluster-name") {
          se.Capability.push_back("information.discovery.resource");
          se.InterfaceName = "org.nordugrid.ldapng";
        }
        else if (itMds->Child(i).Name() == "nordugrid-se-name") {
          se.Capability.push_back("information.discovery.resource");
          se.InterfaceName = "org.nordugrid.ldapng";
        }

        seList.push_back(se);
      }
    }

    s = EndpointQueryingStatus::SUCCESSFUL;
    return s;
  }

} // namespace Arc
