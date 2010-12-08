#ifndef __ARC_SEC_ARGUSPEP_H__
#define __ARC_SEC_ARGUSPEP_H__

#include <arc/ArcConfig.h>
#include <arc/XMLNode.h>
#include <arc/message/Message.h>
#include <arc/message/SecHandler.h>
#include <arc/security/PDP.h>
#include <pep/pep.h>

namespace ArcSec {

class ArgusPEP : public SecHandler {
 
  private:

  typedef enum {
     conversion_subject,
     conversion_direct
   } conversion_type;

   
  std::string pepdlocation;
  std::list<std::string> select_attrs;
  std::list<std::string> reject_attrs;
  conversion_type conversion;
  PEP * pep_handle ;
  bool res;
  pep_error_t pep_rc;
  bool valid_; 
  static Arc::Logger logger;
  // XACML request and response
  // xacml_request_t * request;
  // xacml_response_t * response;

  public:
    ArgusPEP(Arc::Config *cfg);
    ArgusPEP(void);
    virtual ~ArgusPEP(void);
    virtual bool Handle(Arc::Message* msg) const ;
    operator bool(void) { return valid_; };
    bool operator!(void) { return !valid_; };
  
  private:
    int create_xacml_request(xacml_request_t** request, const char * subjectid, const char * resourceid, const char * actionid) const ;
    int create_xacml_request(std::list<xacml_request_t*>& requests,Arc::XMLNode arcreq) const;
 // const char * decision_tostring(xacml_decision_t decision);
 // const char * fulfillon_tostring(xacml_fulfillon_t fulfillon);

  };

} // namespace ArcSec

#endif /* __ARC_SEC_ArgusPEP_H__ */
