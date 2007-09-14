#ifndef __ARC_DELEGATIONINTERFACE_H__
#define __ARC_DELEGATIONINTERFACE_H__

#include <string>
//#include <list>
//#include <glibmm/thread.h>
//#include "../../../libs/common/XMLNode.h"
//#include "../../../hed/libs/message/SOAPEnvelope.h"

namespace Arc {



/// This class manages private key of delegation procedure.
/** */
class DelegationConsumer {
 protected:
  void* key_;
  bool Generate(void);
  void LogError(void);
 public:
  /** */
  DelegationConsumer(void);
  DelegationConsumer(const std::string& content);
  ~DelegationConsumer(void);
  operator bool(void) { return key_ != NULL; };
  bool operator!(void) { return key_ == NULL; };
  const std::string& ID(void);
  bool Backup(std::string& content);
  bool Restore(const std::string& content);
  bool Request(std::string& content);
};

class DelegationProvider {
  void* key_;
  void* cert_;
  void* chain_;
  void LogError(void);
  void CleanError(void);
 public:
  DelegationProvider(const std::string& credentials);
  ~DelegationProvider(void);
  std::string Delegate(const std::string& request);
};

} // namespace Arc

#endif /* __ARC_DELEGATIONINTERFACE_H__ */
