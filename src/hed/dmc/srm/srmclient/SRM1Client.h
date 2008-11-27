#ifndef __HTTPSD_SRM1_CLIENT_H__
#define __HTTPSD_SRM1_CLIENT_H__

#include "SRMClient.h"

#include "srm1_soapH.h"

const static SOAP_NMAC struct Namespace srm1_soap_namespaces[] =
{
  {"SOAP-ENV", "http://schemas.xmlsoap.org/soap/envelope/", "http://www.w3.org/*/soap-envelope", NULL},
  {"SOAP-ENC", "http://schemas.xmlsoap.org/soap/encoding/", "http://www.w3.org/*/soap-encoding", NULL},
  {"xsi", "http://www.w3.org/2001/XMLSchema-instance", "http://www.w3.org/*/XMLSchema-instance", NULL},
  {"xsd", "http://www.w3.org/2001/XMLSchema", "http://www.w3.org/*/XMLSchema", NULL},
  {"SRMv1Type", "http://www.themindelectric.com/package/diskCacheV111.srm/", NULL, NULL},
  {"SRMv1Meth", "http://tempuri.org/diskCacheV111.srm.server.SRMServerV1", NULL, NULL},
  {NULL, NULL, NULL, NULL}
};

// put here to avoid multiple definition errors

//const static SOAP_NMAC struct Namespace srm_soap_namespaces[] =
//{
//  {"SOAP-ENV", "http://schemas.xmlsoap.org/soap/envelope/", "http://www.w3.org/*/soap-envelope", NULL},
//  {"SOAP-ENC", "http://schemas.xmlsoap.org/soap/encoding/", "http://www.w3.org/*/soap-encoding", NULL},
//  {"xsi", "http://www.w3.org/2001/XMLSchema-instance", "http://www.w3.org/*/XMLSchema-instance", NULL},
//  {"xsd", "http://www.w3.org/2001/XMLSchema", "http://www.w3.org/*/XMLSchema", NULL},
//  {"SRMv1Type", "http://www.themindelectric.com/package/diskCacheV111.srm/", NULL, NULL},
//  {"SRMv1Meth", "http://tempuri.org/diskCacheV111.srm.server.SRMServerV1", NULL, NULL},
//  {NULL, NULL, NULL, NULL}
//};
  
//namespace Arc {
  
  class SRM1Client: public SRMClient {
   private:
    struct soap soapobj;
    bool acquire(SRMClientRequest& req,std::list<std::string>& urls);
    static Arc::Logger logger;
    
   public:
    SRM1Client(std::string url);
    ~SRM1Client(void);
    
    // not supported in v1
    SRMReturnCode ping(std::string& version,
                       bool report_error = true)
      {return SRM_ERROR_NOT_SUPPORTED;};
    // not supported in v1
    SRMReturnCode getSpaceTokens(std::list<std::string>& tokens,
                                 std::string description = "")
      {return SRM_ERROR_NOT_SUPPORTED;};
    // not supported in v1
    SRMReturnCode getRequestTokens(std::list<std::string>& tokens,
                                   std::string description = "")
      {return SRM_ERROR_NOT_SUPPORTED;};
    // not supported in v1
    SRMReturnCode requestBringOnline(SRMClientRequest& req) {return SRM_ERROR_NOT_SUPPORTED;};
    // not supported in v1
    SRMReturnCode requestBringOnlineStatus(SRMClientRequest& req) {return SRM_ERROR_NOT_SUPPORTED;};
  
    bool getTURLs(SRMClientRequest& req, std::list<std::string>& urls);
    bool putTURLs(SRMClientRequest& req, std::list<std::string>& urls,unsigned long long size = 0);
    bool releaseGet(SRMClientRequest& req);
    bool releasePut(SRMClientRequest& req);
    bool release(SRMClientRequest& req);
    bool abort(SRMClientRequest& req);
    bool info(SRMClientRequest& req, std::list<struct SRMFileMetaData>& metadata, const int recursive = 0);
    bool remove(SRMClientRequest& req);
    bool copy(SRMClientRequest& req, const std::string& source);
    
  };

//} // namespace Arc

#endif // __HTTPSD_SRM1_CLIENT_H__
