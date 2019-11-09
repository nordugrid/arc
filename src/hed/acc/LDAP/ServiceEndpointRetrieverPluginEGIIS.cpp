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

#include "ServiceEndpointRetrieverPluginEGIIS.h"

namespace Arc {

  Logger ServiceEndpointRetrieverPluginEGIIS::logger(Logger::getRootLogger(), "ServiceEndpointRetrieverPlugin.EGIIS");

  bool ServiceEndpointRetrieverPluginEGIIS::isEndpointNotSupported(const Endpoint& endpoint) const {
    const std::string::size_type pos = endpoint.URLString.find("://");
    return pos != std::string::npos && lower(endpoint.URLString.substr(0, pos)) != "ldap";
  }

  EndpointQueryingStatus ServiceEndpointRetrieverPluginEGIIS::Query(const UserConfig& uc,
                                                                    const Endpoint& rEndpoint,
                                                                    std::list<Endpoint>& seList,
                                                                    const EndpointQueryOptions<Endpoint>&) const {
    EndpointQueryingStatus s(EndpointQueryingStatus::STARTED);

    if (isEndpointNotSupported(rEndpoint)) {
      return s;
    }
    URL url((rEndpoint.URLString.find("://") == std::string::npos ? "ldap://" : "") + rEndpoint.URLString, false, 2135);
    url.ChangeLDAPScope(URL::base);
    // This is not needed for EGIIS
    // It was needed for the original ancient Globus GIIS
    // There are no such installations around any more (as far as we know)
    // url.AddLDAPAttribute("giisregistrationstatus");
    if (!url) return EndpointQueryingStatus::FAILED;

    DataBuffer buffer;
    DataHandle handler(url, uc);

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
        if (itMds->Child(i).Name() != "Mds-Vo-name" &&
            itMds->Child(i).Name() != "nordugrid-cluster-name" &&
            itMds->Child(i).Name() != "nordugrid-se-name") {
          // Unknown entry
          logger.msg(DEBUG, "Unknown entry in EGIIS (%s)", itMds->Child(i).Name());
          continue;
        }
            
        if (!itMds->Child(i)["Mds-Service-type"] ||
            !itMds->Child(i)["Mds-Service-hn"] ||
            !itMds->Child(i)["Mds-Service-port"] ||
            !itMds->Child(i)["Mds-Service-Ldap-suffix"]) {
          logger.msg(DEBUG, "Entry in EGIIS is missing one or more of the attributes 'Mds-Service-type', 'Mds-Service-hn', 'Mds-Service-port' and/or 'Mds-Service-Ldap-suffix'");
          continue;
        }
        
        Endpoint se((std::string)itMds->Child(i)["Mds-Service-type"] + "://" +
                           (std::string)itMds->Child(i)["Mds-Service-hn"] + ":" +
                           (std::string)itMds->Child(i)["Mds-Service-port"] + "/" +
                           (std::string)itMds->Child(i)["Mds-Service-Ldap-suffix"]);
        if (itMds->Child(i).Name() == "Mds-Vo-name") {
          se.Capability.insert("information.discovery.registry");
          se.InterfaceName = supportedInterfaces.empty()?std::string(""):supportedInterfaces.front();
        }
        else if (itMds->Child(i).Name() == "nordugrid-cluster-name") {
          se.Capability.insert("information.discovery.resource");
          se.InterfaceName = "org.nordugrid.ldapng";
        }
        else if (itMds->Child(i).Name() == "nordugrid-se-name") {
          se.Capability.insert("information.discovery.resource");
          se.InterfaceName = "org.nordugrid.ldapng";
        }

        seList.push_back(se);
      }
    }

    s = EndpointQueryingStatus::SUCCESSFUL;
    return s;
  }

} // namespace Arc
