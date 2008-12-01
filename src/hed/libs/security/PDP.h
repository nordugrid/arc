#ifndef __ARC_SEC_PDP_H__
#define __ARC_SEC_PDP_H__

#include <string>
#include <arc/message/Message.h>
#include <arc/loader/Plugin.h>
#include <arc/ArcConfig.h>
#include <arc/Logger.h>

namespace ArcSec {
  //AuthzRequest, AuthzRequestSection, internal structure for request context 
  /** These structure are based on the request schema for PDP, so far it can apply to
  * the ArcPDP's request schema, see src/hed/pdc/Request.xsd and src/hed/pdc/Request.xml. It could also apply to 
  * the XACMLPDP's request schema, since the difference is minor.
  * 
  * Another approach is, the service composes/marshalls the xml structure directly, then the service should use 
  * difference code to compose for ArcPDP's request schema and XACMLPDP's schema, which is not so good. 
  */
  typedef struct {
    std::string value;
    std::string id;
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
      while(!(request.empty())) { request.pop_back(); }
      request.push_back(requestitem); 
    };
    void SetRequestItem(std::list<ArcSec::AuthzRequest> req) { 
      while(!(request.empty())) { request.pop_back(); }
      request = req;
    };
    int RequestItemSize() { return (int)(request.size()); };
    ArcSec::AuthzRequest& GetRequestItem(int n) { 
      std::list<ArcSec::AuthzRequest>::iterator it, ret;
      it = request.begin();
      for(int i = 0; i<=n; i++) {ret = it; it++;}
      return (*ret); 
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
    std::list<std::string>& GetPolicyLocation() { return policylocation; }; 
    virtual ~PDPConfigContext(void) {
      while(!(request.empty())) { request.pop_back(); }
    };
  };

  /// Base class for Policy Decision Point plugins
  /** This virtual class defines method isPermitted() which processes
    security related information/attributes in Message and makes security 
    decision - permit (true) or deny (false). 
    Configuration of PDP is consumed during creation of instance
    through XML subtree fed to constructor. */
  class PDP: public Arc::Plugin {
   public:
    PDP(Arc::Config* cfg) { if(cfg) id_=(std::string)(cfg->Attribute("id")); };
    virtual ~PDP() {};
    virtual bool isPermitted(Arc::Message *msg) = 0;
    void SetId(std::string& id) { id_ = id; };
    std::string GetId() { return id_; };

   protected:
    std::string id_;
    static Arc::Logger logger;
  };

  #define PDPPluginKind ("HED:PDP")

  class PDPPluginArgument: public Arc::PluginArgument {
   private:
    Arc::Config* config_;
   public:
    PDPPluginArgument(Arc::Config* config):config_(config) { };
    virtual ~PDPPluginArgument(void) { };
    operator Arc::Config* (void) { return config_; };
  };

} // namespace ArcSec

#endif /* __ARC_SEC_PDP_H__ */
