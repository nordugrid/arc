#ifndef __ARC_SEC_ARGUSPDPCLIENT_H__
#define __ARC_SEC_ARGUSPDPCLIENT_H__

#include <arc/ArcConfig.h>
#include <arc/XMLNode.h>
#include <arc/message/Message.h>
#include <arc/message/MessageAttributes.h>
#include <arc/message/MessageAuth.h>
#include <arc/message/SecHandler.h>
#include <arc/security/PDP.h>
#include <arc/XMLNode.h>

namespace ArcSec {

class ArgusPDPClient : public SecHandler {
 
  private:

    typedef enum {
      conversion_subject,
      conversion_cream,
      conversion_emi
    } conversion_type;

   
    std::string pdpdlocation;
    std::string keypath;
    std::string certpath;
    std::string capath;
    std::list<std::string> select_attrs;
    std::list<std::string> reject_attrs;
    conversion_type conversion;
    bool accept_mapping;
    bool accept_notapplicable;
    bool valid_; 
    static Arc::Logger logger;

  public:
    ArgusPDPClient(Arc::Config *cfg,Arc::PluginArgument* parg);
    ArgusPDPClient(void);
    virtual ~ArgusPDPClient(void);
    virtual SecHandlerStatus Handle(Arc::Message* msg) const ;
    operator bool(void) { return valid_; };
    bool operator!(void) { return !valid_; };
  
  private:
    int create_xacml_request(Arc::XMLNode& request, const char * subjectid, const char * resourceid, const char * actionid) const;
    int create_xacml_request_cream(Arc::XMLNode& request, std::list<Arc::MessageAuth*> auths, Arc::MessageAttributes* attrs, Arc::XMLNode operation) const;
    int create_xacml_request_emi(Arc::XMLNode& request, std::list<Arc::MessageAuth*> auths, Arc::MessageAttributes* attrs, Arc::XMLNode operation) const;
 // const char * decision_tostring(xacml_decision_t decision);
 // const char * fulfillon_tostring(xacml_fulfillon_t fulfillon);

  };

} // namespace ArcSec

#endif /* __ARC_SEC_ARGUSPDPCLIENT_H__ */
