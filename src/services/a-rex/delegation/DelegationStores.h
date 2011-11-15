#include <string>
#include <map>

#include <arc/Thread.h>
#include <arc/message/SOAPEnvelope.h>
#include <arc/XMLNode.h>

namespace ARex {

class DelegationStore;

class DelegationStores {
 private:
  Glib::Mutex lock_;
  std::map<std::string,DelegationStore*> stores_;
 public:
  DelegationStores(void);
  DelegationStores(const DelegationStores&) { };
  ~DelegationStores(void);
  DelegationStore& operator[](const std::string& path); 
  bool MatchNamespace(const Arc::SOAPEnvelope& in);
  bool Process(const std::string& path,const Arc::SOAPEnvelope& in,Arc::SOAPEnvelope& out,const std::string& client,std::string& credentials);
  bool DelegatedToken(const std::string& path,Arc::XMLNode token,const std::string& client,std::string& credentials);
};

} // namespace ARex

