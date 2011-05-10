#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string>
#include <vector>

#include <arc/URL.h>
#include <arc/Logger.h>

#include "auth.h"

namespace ArcSHCLegacy {

#define LDAP_CONNECT_TIMEOUT 10
#define LDAP_QUERY_TIMEOUT 20
#define LDAP_RESULT_TIMEOUT 60

static Arc::Logger logger(Arc::Logger::getRootLogger(),"AuthUserLDAP");

/*
class result_t {
 public:
  std::string subject;
  int decision;
  result_t(const char* s):subject(s),decision(AAA_NO_MATCH) {};
};

static void result_callback(const std::string & attr,const std::string & value,void * ref) {
  result_t* r = (result_t*)ref;
  if(r->decision != AAA_NO_MATCH) return;
  if(attr == "description") {
    if(strncmp("subject=",value.c_str(),8) == 0) {
      const char* s = value.c_str()+8;
      for(;*s;s++) if(*s != ' ') break;
      if(strcmp(s,r->subject.c_str()) == 0) {
        r->decision=AAA_POSITIVE_MATCH;
      };
    };
  };
}
*/

int AuthUser::match_ldap(const char* line) {
/*
#ifdef HAVE_LDAP
  for(;;) {
    std::string u("");
    int n = gridftpd::input_escaped_string(line,u,' ','"');
    if(n == 0) break;
    line+=n;
	try {
      Arc::URL url(u.c_str());
      if(url.Protocol() != "ldap") return AAA_FAILURE;
      std::string usersn("");
      gridftpd::LdapQuery ldap(url.Host(), url.Port(), false, usersn);
      logger.msg(Arc::INFO, "Connecting to %s:%i", url.Host(), url.Port());
      logger.msg(Arc::INFO, "Quering at %s", url.Path());
      std::vector<std::string> attrs; attrs.push_back("description");
      try {
        ldap.Query(url.Path(),"",attrs,gridftpd::LdapQuery::onelevel);
      } catch (gridftpd::LdapQueryError e) {
        logger.msg(Arc::ERROR, "Failed to query LDAP server %s", u);
        return AAA_FAILURE;
      };
      result_t r(subject.c_str());
      try {
        ldap.Result(&result_callback,&r) ;
      } catch (gridftpd::LdapQueryError e) {
        logger.msg(Arc::ERROR, "Failed to get results from LDAP server %s", u);
        return AAA_FAILURE;
      };
      if(r.decision==AAA_POSITIVE_MATCH) {  // just a placeholder
        default_voms_=NULL;
        default_vo_=NULL;
        default_role_=NULL;
        default_capability_=NULL;
        default_vgroup_=NULL;
      }; 
      return r.decision;
    } catch (std::exception e) {
      return AAA_FAILURE;
    };
  };
  return AAA_NO_MATCH;
#else
  logger.msg(Arc::ERROR, "LDAP authorization is not supported");
  return AAA_FAILURE;
#endif
*/
  logger.msg(Arc::ERROR, "LDAP authorization is not implemented yet");
  return AAA_FAILURE;
}

} // namespace ArcSHCLegacy

