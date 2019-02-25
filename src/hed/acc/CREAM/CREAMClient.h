// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_CREAMCLIENT_H__
#define __ARC_CREAMCLIENT_H__

#include <string>

#include <arc/XMLNode.h>
#include <arc/compute/Job.h>

/** CREAMClient
 * The CREAMClient class is used for commnunicating with a CREAM 2 computing
 * service. CREAM uses WSRF technology with a WSDL specifying the allowed
 * operations. The particular WSDLs can be obtained from the CERN source code
 * repository with names 'org.glite.ce-cream2_service.wsdl' and
 * 'www.gridsite.org-delegation-2.0.0.wsdl', at path 'org.glite.ce.wsdl'. A
 * package for these WSDLs probably also exist.
 */

namespace Arc {

  class ClientSOAP;
  class Logger;
  class MCCConfig;
  class URL;
  class PayloadSOAP;

  class creamJobInfo {
  public:
    std::string id;
    std::string creamURL;
    std::string ISB;
    std::string OSB;
    std::string delegationID;
    
    creamJobInfo& operator=(XMLNode n);
    XMLNode ToXML() const;
  };

  class CREAMClient {
  public:
    CREAMClient(const URL& url, const MCCConfig& cfg, int timeout);
    ~CREAMClient();
    void setDelegationId(const std::string& delegId) {
      this->delegationId = delegId;
    }
    bool createDelegation(const std::string& delegation_id,
                          const std::string& proxy);
    bool registerJob(const std::string& jdl_text, creamJobInfo& info);
    bool startJob(const std::string& jobid);
    bool stat(const std::string& jobid, Job& job);
    bool listJobs(std::list<creamJobInfo>& info);
    bool getJobDesc(const std::string& jobid, std::string& desc);
    bool cancel(const std::string& jobid);
    bool purge(const std::string& jobid);
    bool resume(const std::string& jobid);

  private:
    bool process(PayloadSOAP& req, XMLNode& response, const std::string& actionNS = "http://glite.org/2007/11/ce/cream/");

    std::string action;

    ClientSOAP *client;
    std::string cafile;
    std::string cadir;
    NS cream_ns;
    std::string delegationId;
    static Logger logger;
  };

} // namespace Arc

#endif // __ARC_CREAMCLIENT_H__