// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/StringConv.h>
#include <arc/URL.h>
#include <arc/credential/Credential.h>
#include <arc/data/DataBuffer.h>
#include <arc/data/DataHandle.h>

#include "JobListRetrieverPluginLDAPNG.h"

namespace Arc {

  // Characters to be escaped in LDAP filter according to RFC4515
  static const std::string filter_esc("&|=!><~*/()");

  Logger JobListRetrieverPluginLDAPNG::logger(Logger::getRootLogger(), "JobListRetrieverPlugin.LDAPNG");

  bool JobListRetrieverPluginLDAPNG::isEndpointNotSupported(const Endpoint& endpoint) const {
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
      service += "/Mds-Vo-name=local, o=Grid";
    }
    else if (pos2 == std::string::npos || pos2 > pos3)
      service.insert(pos3, ":2135");
    
    return service;
  }

  EndpointQueryingStatus JobListRetrieverPluginLDAPNG::Query(const UserConfig& uc, const Endpoint& endpoint, std::list<Job>& jobs, const EndpointQueryOptions<Job>&) const {
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

    //Query GRIS for all relevant information
    url.ChangeLDAPScope(URL::subtree);

    // Applying filter. Must be done through EndpointQueryOptions.
    url.ChangeLDAPFilter("(|(nordugrid-job-globalowner=" + escaped_dn + "))");

    DataHandle handler(url, uc);
    DataBuffer buffer;

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

    XMLNode xmlresult(result);
    XMLNodeList xJobs = xmlresult.XPathLookup("//nordugrid-job-globalid[objectClass='nordugrid-job']", NS());
    for (XMLNodeList::iterator it = xJobs.begin(); it != xJobs.end(); ++it) {
      Job j;
      if ((*it)["nordugrid-job-globalid"])
        j.JobID = (std::string)(*it)["nordugrid-job-globalid"];
      if ((*it)["nordugrid-job-jobname"])
        j.Name = (std::string)(*it)["nordugrid-job-jobname"];
      if ((*it)["nordugrid-job-submissiontime"])
        j.LocalSubmissionTime = (std::string)(*it)["nordugrid-job-submissiontime"];

      j.InterfaceName = "org.nordugrid.gridftpjob";
      j.Cluster = url;

      URL infoEndpoint(url);
      infoEndpoint.ChangeLDAPFilter("(nordugrid-job-globalid=" +
                                    escape_chars((std::string)(*it)["nordugrid-job-globalid"],filter_esc,'\\',false,escape_hex) + ")");
      infoEndpoint.ChangeLDAPScope(URL::subtree);
      j.IDFromEndpoint = infoEndpoint.fullstr();

      jobs.push_back(j);
    }

    s = EndpointQueryingStatus::SUCCESSFUL;

    return s;
  }

} // namespace Arc
