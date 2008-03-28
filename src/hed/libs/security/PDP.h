#ifndef __ARC_SEC_PDP_H__
#define __ARC_SEC_PDP_H__

#include <string>
#include <arc/message/Message.h>
#include <arc/ArcConfig.h>
#include <arc/Logger.h>

namespace ArcSec {
  typedef struct {
    std::string value;
    std::string type;
    std::string issuer;
  } AuthzRequestSection;
  typedef struct {
    std::list<ArcSec::AuthzRequestSection> subject;
    std::list<ArcSec::AuthzRequestSection> resource;
    std::list<ArcSec::AuthzRequestSection> action;
    std::list<ArcSec::AuthzRequestSection> context;
  } AuthzRequest;

  class PDPConfigContext:public Arc::MessageContextElement {
   private:
    std::list<ArcSec::AuthzRequest> request;
    std::list<std::string> policylocation;
    
   public:
    PDPConfigContext() {};
    PDPConfigContext(std::list<ArcSec::AuthzRequest> req, std::string& policy) {request = req; policylocation.push_back(policy); };
    PDPConfigContext(std::list<ArcSec::AuthzRequest> req, std::list<std::string> policy) {request = req; policylocation = policy; };
    void AddRequestItem(ArcSec::AuthzRequest requestitem) { request.push_back(requestitem); };
    void SetRequestItem(ArcSec::AuthzRequest requestitem) { 
      std::list<ArcSec::AuthzRequest>::iterator it1 = request.begin();
      std::list<ArcSec::AuthzRequest>::iterator it2 = request.end();
      request.erase(it1, it2); 
      request.push_back(requestitem); 
    };
    void SetRequestItem(std::list<ArcSec::AuthzRequest> req) { 
      std::list<ArcSec::AuthzRequest>::iterator it1 = request.begin();
      std::list<ArcSec::AuthzRequest>::iterator it2 = request.end();
      request.erase(it1, it2);
      request = req;
    };
    int RequestItemSize() { (int) request.size(); };
    ArcSec::AuthzRequest GetRequestItem(int n) { 
      std::list<ArcSec::AuthzRequest>::iterator it;
      it = request.begin();
      for(int i = 0; i<=n; i++) it++;
      return (*it); 
    };
    void AddPolicyLocation(std::string& policy) { policylocation.push_back(policy); };
    void SetPolicyLocation(std::list<std::string> policy) { 
      std::list<std::string>::iterator it1 = policylocation.begin();
      std::list<std::string>::iterator it2 = policylocation.end(); 
      policylocation.erase(it1, it2); 
      policylocation = policy; 
    };
    void SetPolicyLocation(std::string& policy) { 
      std::list<std::string>::iterator it1 = policylocation.begin();
      std::list<std::string>::iterator it2 = policylocation.end();  
      policylocation.erase(it1, it2);
      policylocation.push_back(policy); 
    };
    virtual ~PDPConfigContext(void) {};
  };

  /// Base class for Policy Decisoion Point plugins
  /** This virtual class defines method isPermitted() which processes
    security related information/attributes in Message and makes security 
    decision - permit (true) or deny (false). 
    Configuration of PDP is consumed during creation of instance
    through XML subtree fed to constructor. */
  class PDP {
   public:
    PDP(Arc::Config*) {};
    virtual ~PDP() {};
    virtual bool isPermitted(Arc::Message *msg) = 0;
   protected:
    static Arc::Logger logger;
  };

} // namespace ArcSec

#endif /* __ARC_SEC_PDP_H__ */
