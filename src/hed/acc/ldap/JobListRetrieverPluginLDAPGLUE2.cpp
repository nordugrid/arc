// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/StringConv.h>
#include <arc/compute/ComputingServiceRetriever.h>

#include "JobListRetrieverPluginLDAPGLUE2.h"

namespace Arc {

  // Characters to be escaped in LDAP filter according to RFC4515
  static const std::string filter_esc("&|=!><~*/()");

  Logger JobListRetrieverPluginLDAPGLUE2::logger(Logger::getRootLogger(), "JobListRetrieverPlugin.LDAPGLUE2");

  bool JobListRetrieverPluginLDAPGLUE2::isEndpointNotSupported(const Endpoint& endpoint) const {
    const std::string::size_type pos = endpoint.URLString.find("://");
    return pos != std::string::npos && lower(endpoint.URLString.substr(0, pos)) != "ldap";
  }

  EndpointQueryingStatus JobListRetrieverPluginLDAPGLUE2::Query(const UserConfig& uc, const Endpoint& endpoint, std::list<Job>& jobs, const EndpointQueryOptions<Job>&) const {
    EndpointQueryingStatus s(EndpointQueryingStatus::FAILED);

    ComputingServiceRetriever csr(uc);
    csr.addEndpoint(endpoint);
    csr.wait();
    
    EntityContainer<Job> container;
    JobListRetriever jlr(uc);
    jlr.addConsumer(container);
    
    for (std::list<ComputingServiceType>::const_iterator it = csr.begin(); it != csr.end(); ++it) {
      for (std::map<int, ComputingEndpointType>::const_iterator ite = it->ComputingEndpoint.begin(); ite != it->ComputingEndpoint.end(); ite++) {
        Endpoint e(*(ite->second));
        if (e.HasCapability(Endpoint::JOBLIST) &&
            e.InterfaceName != "org.nordugrid.ldapglue2" &&
          // the wsrfglue2 job list retriever is not prepared to coexist with the others, so rather skip it
            e.InterfaceName != "org.nordugrid.wsrfglue2") jlr.addEndpoint(e);
      }
    }
    jlr.wait();
    jobs.insert(jobs.end(), container.begin(), container.end());
    s = EndpointQueryingStatus::SUCCESSFUL;
    return s;
  }

} // namespace Arc
