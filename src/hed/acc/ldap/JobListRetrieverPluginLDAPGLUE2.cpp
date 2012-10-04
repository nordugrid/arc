// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/StringConv.h>
#include <arc/URL.h>
#include <arc/credential/Credential.h>
#include <arc/data/DataBuffer.h>
#include <arc/data/DataHandle.h>

#include "JobListRetrieverPluginLDAPGLUE2.h"

namespace Arc {

  // Characters to be escaped in LDAP filter according to RFC4515
  static const std::string filter_esc("&|=!><~*/()");

  Logger JobListRetrieverPluginLDAPGLUE2::logger(Logger::getRootLogger(), "JobListRetrieverPlugin.LDAPGLUE2");

  bool JobListRetrieverPluginLDAPGLUE2::isEndpointNotSupported(const Endpoint& endpoint) const {
    const std::string::size_type pos = endpoint.URLString.find("://");
    return pos != std::string::npos && lower(endpoint.URLString.substr(0, pos)) != "ldap";
  }

  static URL CreateURL(std::string service) {
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
      service += "/o=glue";
    }
    else if (pos2 == std::string::npos || pos2 > pos3)
      service.insert(pos3, ":2135");
    
    return service;
  }

  EndpointQueryingStatus JobListRetrieverPluginLDAPGLUE2::Query(const UserConfig& uc, const Endpoint& endpoint, std::list<Job>& jobs, const EndpointQueryOptions<Job>&) const {
    EndpointQueryingStatus s(EndpointQueryingStatus::FAILED);

    URL url(CreateURL(endpoint.URLString));
    if (!url) {
      return s;
    }

    //Create credential object in order to get the user DN
    const std::string *certpath, *keypath;
    if (uc.ProxyPath().empty()) {
      certpath = &uc.CertificatePath();
      keypath  = &uc.KeyPath();
    }
    else {
      certpath = &uc.ProxyPath();
      keypath  = &uc.ProxyPath();
    }
    std::string emptycadir;
    std::string emptycafile;
    Credential credential(*certpath, *keypath, emptycadir, emptycafile);
    std::string escaped_dn = escape_chars(credential.GetIdentityName(), filter_esc, '\\', false, escape_hex);
    url.ChangeLDAPScope(URL::subtree);
    url.ChangeLDAPFilter("(|(objectclass=GLUE2ComputingEndpoint)(GLUE2ComputingActivityOwner=" + escaped_dn + "))");

    DataBuffer buffer;
    DataHandle handler(url, uc);

    if (!handler) {
      logger.msg(INFO, "Can't create information handle - is the ARC ldap DMC plugin available?");
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
      return s;
    }

    url.ChangeLDAPFilter("");
    XMLNode xmlresult(result);
    XMLNodeList xEndpoints = xmlresult.XPathLookup("/LDAPQueryResult/o/GLUE2GroupID/GLUE2ServiceID/GLUE2EndpointID", NS());
    for (XMLNodeList::iterator it = xEndpoints.begin(); it != xEndpoints.end(); ++it) {
      XMLNodeList xJobs = it->XPathLookup("//GLUE2GroupID/GLUE2ActivityID", NS());
      for (XMLNodeList::iterator itj = xJobs.begin(); itj != xJobs.end(); ++itj) {
        std::string ID = (std::string)(*itj)["GLUE2ComputingActivityIDFromEndpoint"];
        std::string Name = (std::string)(*itj)["GLUE2EntityName"];
        std::string ResourceInfoURL = url.fullstr();
        std::string ResourceInfoInterfaceName = "org.nordugrid.ldapglue2";
        std::string ActivityInfoURL = "";
        std::string ActivityInfoInterfaceName = "";
        std::string ActivityManagementURL = (std::string)(*it)["GLUE2EndpointURL"];
        std::string ActivityManagementInterfaceName = (std::string)(*it)["GLUE2EndpointInterfaceName"];

        if (ID.find("://") == std::string::npos) {
          logger.msg(VERBOSE, "Job ID (%s) will be extended to a URL: %s", ID, ActivityManagementURL + "/" + ID);
          ID = ActivityManagementURL + "/" + ID;
        }

        logger.msg(DEBUG, "Job - ID: %s", ID);
        logger.msg(DEBUG, "Job - Name: %s", Name);
        logger.msg(DEBUG, "Job - ResourceInfoURL: %s", ResourceInfoURL);
        logger.msg(DEBUG, "Job - ResourceInfoInterfaceName: %s", ResourceInfoInterfaceName);
        logger.msg(DEBUG, "Job - ActivityInfoURL: %s", ActivityInfoURL);
        logger.msg(DEBUG, "Job - ActivityInfoInterfaceName: %s", ActivityInfoInterfaceName);
        logger.msg(DEBUG, "Job - ActivityManagementURL: %s", ActivityManagementURL);
        logger.msg(DEBUG, "Job - ActivityManagementInterfaceName: %s", ActivityManagementInterfaceName);
        Job j;
        j.JobID = ID;
        j.IDFromEndpoint = ID;
        j.Name = Name;
        j.Cluster = ActivityManagementURL;
        j.InterfaceName = ActivityManagementInterfaceName;
        jobs.push_back(j);
      }
      
    }

    s = EndpointQueryingStatus::SUCCESSFUL;

    return s;
  }

} // namespace Arc
