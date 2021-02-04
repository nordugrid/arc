// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <openssl/sha.h>
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

  EndpointQueryingStatus JobListRetrieverPluginLDAPNG::Query(const UserConfig& uc, const Endpoint& endpoint, std::list<Job>& jobs, const EndpointQueryOptions<Job>&) const {
    EndpointQueryingStatus s(EndpointQueryingStatus::FAILED);

    if (isEndpointNotSupported(endpoint)) {
      return s;
    }
    URL url((endpoint.URLString.find("://") == std::string::npos ? "ldap://" : "") + endpoint.URLString, false, 2135, "/Mds-Vo-name=local,o=Grid");
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
    std::string dn = credential.GetIdentityName();
    std::string escaped_dn = escape_chars(dn, filter_esc, '\\', false, escape_hex);
    std::string hashed_dn;
    {
      unsigned char hash[SHA512_DIGEST_LENGTH];
      SHA512((unsigned char const *)dn.c_str(), dn.length(), hash);
      for(int idx = 0; idx < SHA512_DIGEST_LENGTH; ++idx)
        hashed_dn += inttostr((unsigned int)hash[idx], 16, 2);
    }

    //Query GRIS for all relevant information
    url.ChangeLDAPScope(URL::subtree);

    // Applying filter. Must be done through EndpointQueryOptions.
    url.ChangeLDAPFilter("(|(nordugrid-job-globalowner=" + escaped_dn + ")(nordugrid-job-globalowner=" + hashed_dn + ")(objectClass=nordugrid-cluster))");

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

    XMLNode xmlresult(result);
    XMLNodeList xContactStrings = xmlresult.XPathLookup("//nordugrid-cluster-contactstring", NS());
    if (xContactStrings.empty()) {
      return s;
    }

    std::string ContactString = (std::string)xContactStrings.front();
    XMLNodeList xJobs = xmlresult.XPathLookup("//nordugrid-job-globalid[objectClass='nordugrid-job']", NS());
    for (XMLNodeList::iterator it = xJobs.begin(); it != xJobs.end(); ++it) {
      Job j;
      if ((*it)["nordugrid-job-comment"]) {
        std::string comment = (std::string)(*it)["nordugrid-job-comment"];
        std::string submittedvia = "SubmittedVia=";
        if (comment.compare(0, submittedvia.length(), submittedvia) == 0) {
          std::string interfacename = comment.substr(submittedvia.length());
          if (interfacename != "org.nordugrid.gridftpjob") {
            logger.msg(DEBUG, "Skipping retrieved job (%s) because it was submitted via another interface (%s).", (std::string)(*it)["nordugrid-job-globalid"], interfacename);
            continue;
          }
        }
      }
      if ((*it)["nordugrid-job-jobname"])
        j.Name = (std::string)(*it)["nordugrid-job-jobname"];
      if ((*it)["nordugrid-job-submissiontime"])
        j.LocalSubmissionTime = (std::string)(*it)["nordugrid-job-submissiontime"];

      URL infoEndpoint(url);
      infoEndpoint.ChangeLDAPFilter("(nordugrid-job-globalid=" +
                                    escape_chars((std::string)(*it)["nordugrid-job-globalid"],filter_esc,'\\',false,escape_hex) + ")");
      infoEndpoint.ChangeLDAPScope(URL::subtree);

      // Proposed mandatory attributes for ARC 3.0
      j.JobID = (std::string)(*it)["nordugrid-job-globalid"];
      j.ServiceInformationURL = url;
      j.ServiceInformationURL.ChangeLDAPFilter("");
      j.ServiceInformationInterfaceName = "org.nordugrid.ldapng";
      j.JobStatusURL = infoEndpoint;
      j.JobStatusInterfaceName = "org.nordugrid.ldapng";
      j.JobManagementURL = URL(ContactString);
      j.JobManagementInterfaceName = "org.nordugrid.gridftpjob";
      j.IDFromEndpoint = j.JobID.substr(ContactString.length()+1);
      j.StageInDir = URL(j.JobID);
      j.StageOutDir = URL(j.JobID);
      j.SessionDir = URL(j.JobID);

      jobs.push_back(j);
    }

    s = EndpointQueryingStatus::SUCCESSFUL;

    return s;
  }

} // namespace Arc
