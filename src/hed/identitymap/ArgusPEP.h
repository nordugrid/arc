#ifndef __ARC_SEC_ARGUSPEP_H__
#define __ARC_SEC_ARGUSPEP_H__

#include <arc/ArcConfig.h>
#include <arc/XMLNode.h>
#include <arc/message/Message.h>
#include <arc/message/MessageAttributes.h>
#include <arc/message/MessageAuth.h>
#include <arc/message/SecHandler.h>
#include <arc/security/PDP.h>
#include <arc/XMLNode.h>

#include <argus/pep.h>

namespace ArcSec {

class ArgusPEP : public SecHandler {
 
  private:

    typedef enum {
      conversion_subject,
      conversion_direct,
      conversion_cream,
      conversion_emi
    } conversion_type;

   
    std::string pepdlocation;
    int pep_log_level;
    std::string keypath;
    std::string certpath;
    std::string capath;
    std::list<std::string> select_attrs;
    std::list<std::string> reject_attrs;
    conversion_type conversion;
    bool valid_; 
    static Arc::Logger logger;
    // XACML request and response
    // xacml_request_t * request;
    // xacml_response_t * response;
    static int pep_log(int level, const char *fmt, va_list args);

  public:
    ArgusPEP(Arc::Config *cfg);
    ArgusPEP(void);
    virtual ~ArgusPEP(void);
    virtual bool Handle(Arc::Message* msg) const ;
    operator bool(void) { return valid_; };
    bool operator!(void) { return !valid_; };
  
  private:
    int create_xacml_request(xacml_request_t** request, const char * subjectid, const char * resourceid, const char * actionid) const ;
    int create_xacml_request_direct(std::list<xacml_request_t*>& requests,Arc::XMLNode arcreq) const;
    int create_xacml_request_cream(xacml_request_t** request, std::list<Arc::MessageAuth*> auths, Arc::MessageAttributes* attrs, Arc::XMLNode operation) const;
    int create_xacml_request_emi(xacml_request_t** request, std::list<Arc::MessageAuth*> auths, Arc::MessageAttributes* attrs, Arc::XMLNode operation) const;
 // const char * decision_tostring(xacml_decision_t decision);
 // const char * fulfillon_tostring(xacml_fulfillon_t fulfillon);

  };

} // namespace ArcSec

#endif /* __ARC_SEC_ArgusPEP_H__ */
