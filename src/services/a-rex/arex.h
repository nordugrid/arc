#ifndef __ARC_AREX_H__
#define __ARC_AREX_H__

#include <arc/message/PayloadRaw.h>
#include <arc/delegation/DelegationInterface.h>
#include <arc/infosys/InformationInterface.h>
#include <arc/Thread.h>
#include <arc/StringConv.h>
#include <arc/message/Service.h>

#include "FileChunks.h"
#include "grid-manager/GridManager.h"
#include "delegation/DelegationStores.h"
#include "grid-manager/conf/GMConfig.h"

namespace ARex {

class ARexGMConfig;
class ARexConfigContext;
class CountedResourceLock;

class CountedResource {
 friend class CountedResourceLock;
 public:
  CountedResource(int maxconsumers = -1);
  ~CountedResource(void);
  void MaxConsumers(int maxconsumers);
 private:
  Glib::Cond cond_;
  Glib::Mutex lock_;
  int limit_;
  int count_;
  void Acquire(void);
  void Release(void);
};

class OptimizedInformationContainer: public Arc::InformationContainer {
 private:
  bool parse_xml_;
  std::string filename_;
  int handle_;
  Arc::XMLNode doc_;
  Glib::Mutex olock_;
 public:
  OptimizedInformationContainer(bool parse_xml = true);
  ~OptimizedInformationContainer(void);
  int OpenDocument(void);
  void AssignFile(const std::string& filename);
  void Assign(const std::string& xml,const std::string filename = "");
};

#define AREXOP(NAME) Arc::MCC_Status NAME(ARexGMConfig& config,Arc::XMLNode in,Arc::XMLNode out)
class ARexService: public Arc::Service {
 private:
  static void gm_threads_starter(void* arg);
  void gm_threads_starter();
  Arc::MCC_Status cache_get(Arc::Message& outmsg, const std::string& subpath, off_t range_start, off_t range_end, ARexGMConfig& config, bool no_content);
 protected:
  Arc::ThreadRegistry thread_count_;
  static Arc::NS ns_;
  Arc::Logger logger_;
  DelegationStores delegation_stores_;
  OptimizedInformationContainer infodoc_;
  CountedResource infolimit_;
  CountedResource beslimit_;
  CountedResource datalimit_;
  std::string endpoint_;
  bool publishstaticinfo_;
  std::string uname_;
  std::string common_name_;
  std::string long_description_;
  std::string os_name_;
  std::string gmrun_;
  unsigned int infoprovider_wakeup_period_;
  unsigned int all_jobs_count_;
  //Glib::Mutex glue_states_lock_;
  //std::map<std::string,std::string> glue_states_;
  FileChunksList files_chunks_;
  GMConfig config_;
  GridManager* gm_;
  ARexConfigContext* get_configuration(Arc::Message& inmsg);

  // A-REX operations
  AREXOP(CacheCheck);

  /** Update credentials for specified job through A-REX own interface */
  Arc::MCC_Status UpdateCredentials(ARexGMConfig& config,Arc::XMLNode in,Arc::XMLNode out,const std::string& credentials);

  // EMI ES operations
  Arc::MCC_Status ESCreateActivities(ARexGMConfig& config,Arc::XMLNode in,Arc::XMLNode out,const std::string& clientid);
  AREXOP(ESGetResourceInfo);
  AREXOP(ESQueryResourceInfo);
  AREXOP(ESPauseActivity);
  AREXOP(ESResumeActivity);
  AREXOP(ESNotifyService);
  AREXOP(ESCancelActivity);
  AREXOP(ESWipeActivity);
  AREXOP(ESRestartActivity);
  AREXOP(ESListActivities);
  AREXOP(ESGetActivityStatus);
  AREXOP(ESGetActivityInfo);

  // HTTP operations
  Arc::MCC_Status GetJob(Arc::Message& inmsg,Arc::Message& outmsg,ARexGMConfig& config,std::string const& id,std::string const& subpath);
  Arc::MCC_Status GetLogs(Arc::Message& inmsg,Arc::Message& outmsg,ARexGMConfig& config,std::string const& id,std::string const& subpath);
  Arc::MCC_Status GetInfo(Arc::Message& inmsg,Arc::Message& outmsg,ARexGMConfig& config,std::string const& subpath);
  Arc::MCC_Status GetNew(Arc::Message& inmsg,Arc::Message& outmsg,ARexGMConfig& config,std::string const& subpath);
  Arc::MCC_Status GetDelegation(Arc::Message& inmsg,Arc::Message& outmsg,ARexGMConfig& config,std::string const& id,std::string const& subpath);
  Arc::MCC_Status GetCache(Arc::Message& inmsg,Arc::Message& outmsg,ARexGMConfig& config,std::string const& subpath);

  Arc::MCC_Status HeadJob(Arc::Message& inmsg,Arc::Message& outmsg,ARexGMConfig& config,std::string const& id,std::string const& subpath);
  Arc::MCC_Status HeadLogs(Arc::Message& inmsg,Arc::Message& outmsg,ARexGMConfig& config,std::string const& id,std::string const& subpath);
  Arc::MCC_Status HeadInfo(Arc::Message& inmsg,Arc::Message& outmsg,ARexGMConfig& config,std::string const& subpath);
  Arc::MCC_Status HeadNew(Arc::Message& inmsg,Arc::Message& outmsg,ARexGMConfig& config,std::string const& subpath);
  Arc::MCC_Status HeadDelegation(Arc::Message& inmsg,Arc::Message& outmsg,ARexGMConfig& config,std::string const& id,std::string const& subpath);
  Arc::MCC_Status HeadCache(Arc::Message& inmsg,Arc::Message& outmsg,ARexGMConfig& config,std::string const& subpath);

  Arc::MCC_Status PutJob(Arc::Message& inmsg,Arc::Message& outmsg,ARexGMConfig& config,std::string const& id,std::string const& subpath);
  Arc::MCC_Status PutLogs(Arc::Message& inmsg,Arc::Message& outmsg,ARexGMConfig& config,std::string const& id,std::string const& subpath);
  Arc::MCC_Status PutInfo(Arc::Message& inmsg,Arc::Message& outmsg,ARexGMConfig& config,std::string const& subpath);
  Arc::MCC_Status PutNew(Arc::Message& inmsg,Arc::Message& outmsg,ARexGMConfig& config,std::string const& subpath);
  Arc::MCC_Status PutDelegation(Arc::Message& inmsg,Arc::Message& outmsg,ARexGMConfig& config,std::string const& id,std::string const& subpath);
  Arc::MCC_Status PutCache(Arc::Message& inmsg,Arc::Message& outmsg,ARexGMConfig& config,std::string const& subpath);

  Arc::MCC_Status DeleteJob(Arc::Message& inmsg,Arc::Message& outmsg,ARexGMConfig& config,std::string const& id,std::string const& subpath);
  Arc::MCC_Status DeleteLogs(Arc::Message& inmsg,Arc::Message& outmsg,ARexGMConfig& config,std::string const& id,std::string const& subpath);
  Arc::MCC_Status DeleteInfo(Arc::Message& inmsg,Arc::Message& outmsg,ARexGMConfig& config,std::string const& subpath);
  Arc::MCC_Status DeleteNew(Arc::Message& inmsg,Arc::Message& outmsg,ARexGMConfig& config,std::string const& subpath);
  Arc::MCC_Status DeleteDelegation(Arc::Message& inmsg,Arc::Message& outmsg,ARexGMConfig& config,std::string const& id,std::string const& subpath);
  Arc::MCC_Status DeleteCache(Arc::Message& inmsg,Arc::Message& outmsg,ARexGMConfig& config,std::string const& subpath);

  // A-REX faults
  void UnknownActivityIdentifierFault(Arc::XMLNode fault,const std::string& message);
  void UnknownActivityIdentifierFault(Arc::SOAPFault& fault,const std::string& message);
  void InvalidRequestMessageFault(Arc::XMLNode fault,const std::string& element,const std::string& message);
  void InvalidRequestMessageFault(Arc::SOAPFault& fault,const std::string& element,const std::string& message);


  // EMI ES faults
#define ES_MSG_FAULT_HEAD(NAME) \
  void NAME(Arc::XMLNode fault,const std::string& message,const std::string& desc = ""); \
  void NAME(Arc::SOAPFault& fault,const std::string& message,const std::string& desc = "");
#define ES_SIMPLE_FAULT_HEAD(NAME) \
  void NAME(Arc::XMLNode fault,const std::string& message = "",const std::string& desc = ""); \
  void NAME(Arc::SOAPFault& fault,const std::string& message = "",const std::string& desc = "");

  ES_MSG_FAULT_HEAD(ESInternalBaseFault)
  void ESVectorLimitExceededFault(Arc::XMLNode fault,unsigned long limit,const std::string& message = "",const std::string& desc = "");
  void ESVectorLimitExceededFault(Arc::SOAPFault& fault,unsigned long limit,const std::string& message = "",const std::string& desc = "");
  ES_SIMPLE_FAULT_HEAD(ESAccessControlFault);

  ES_SIMPLE_FAULT_HEAD(ESUnsupportedCapabilityFault)
  ES_SIMPLE_FAULT_HEAD(ESInvalidActivityDescriptionSemanticFault)
  ES_SIMPLE_FAULT_HEAD(ESInvalidActivityDescriptionFault)

  ES_SIMPLE_FAULT_HEAD(ESNotSupportedQueryDialectFault)
  ES_SIMPLE_FAULT_HEAD(ESNotValidQueryStatementFault)
  ES_SIMPLE_FAULT_HEAD(ESUnknownQueryFault)
  ES_SIMPLE_FAULT_HEAD(ESInternalResourceInfoFault)
  ES_SIMPLE_FAULT_HEAD(ESResourceInfoNotFoundFault)

  ES_SIMPLE_FAULT_HEAD(ESUnableToRetrieveStatusFault)
  ES_SIMPLE_FAULT_HEAD(ESUnknownAttributeFault)
  ES_SIMPLE_FAULT_HEAD(ESOperationNotAllowedFault)
  ES_SIMPLE_FAULT_HEAD(ESActivityNotFoundFault)
  ES_SIMPLE_FAULT_HEAD(ESInternalNotificationFault)
  ES_SIMPLE_FAULT_HEAD(ESOperationNotPossibleFault)
  ES_SIMPLE_FAULT_HEAD(ESInvalidActivityStateFault)
  ES_SIMPLE_FAULT_HEAD(ESInvalidActivityLimitFault)
  ES_SIMPLE_FAULT_HEAD(ESInvalidParameterFault)

 public:
  ARexService(Arc::Config *cfg,Arc::PluginArgument *parg);
  virtual ~ARexService(void);
  virtual Arc::MCC_Status process(Arc::Message& inmsg,Arc::Message& outmsg);

  // HTTP paths
  static char const* InfoPath;
  static char const* LogsPath;
  static char const* NewPath;
  static char const* DelegationPath;
  static char const* CachePath;

  // Convenience methods
  static Arc::MCC_Status make_empty_response(Arc::Message& outmsg);
  static Arc::MCC_Status make_fault(Arc::Message& outmsg);
  static Arc::MCC_Status make_http_fault(Arc::Message& outmsg,int code,const char* resp);
  static Arc::MCC_Status make_soap_fault(Arc::Message& outmsg,const char* resp = NULL);
  static Arc::MCC_Status extract_content(Arc::Message& inmsg, std::string& content,uint32_t size_limit = 0);

  void InformationCollector(void);
  virtual bool RegistrationCollector(Arc::XMLNode &doc);
  virtual std::string getID();
  void StopChildThreads(void);
};

} // namespace ARex

#define HTTP_ERR_NOT_SUPPORTED (501)
#define HTTP_ERR_FORBIDDEN (403)

#endif

