#include <list>

#include <arc/URL.h>
#include <arc/client/ClientInterface.h>

#include "JobRecord.h"

//#include "../client/client.h"

//#include "logger_soapStub.h"
//#include "logger2_soapStub.h"

//extern struct Namespace logger_soap_namespaces[];
//extern struct Namespace logger2_soap_namespaces[];

namespace ARex {

class LoggerClient {
 private:
  Arc::URL url;
  Arc::ClientSOAP* client;
  //struct soap soap;
  bool SameContact(const char* url);
  bool NewURL(const char* url);
 public:
  LoggerClient(void);
  ~LoggerClient(void);
  bool Initialized(void);
  bool ReportV2(const char* url,std::list<JobRecord>& info);
  //bool Report(const char* url,std::list<nl2__UsageRecord>& info);
  //bool ReportV1(const char* url,std::list<nl2__UsageRecord>& info);
  //bool ReportV2(const char* url,std::list<nl2__UsageRecord>& info);
  //bool Query(const char* url,const char* q,unsigned long long offset,unsigned long long size,std::list<nl2__UsageRecord>& info);
  //bool QueryV1(const char* url,const char* q,unsigned long long offset,std::list<nl2__UsageRecord>& info);
  //bool QueryV2(const char* url,const char* q,unsigned long long offset,unsigned long long size,std::list<nl2__UsageRecord>& info);
};

}

