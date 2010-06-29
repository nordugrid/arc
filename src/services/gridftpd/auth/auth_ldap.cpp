#include <string>
#include <vector>

#include <arc/URL.h>
#include "../misc/ldapquery.h"
#include "../misc/escaped.h"
#include "auth.h"

#define LDAP_CONNECT_TIMEOUT 10
#define LDAP_QUERY_TIMEOUT 20
#define LDAP_RESULT_TIMEOUT 60
#define olog std::cerr

class result_t {
 public:
  std::string subject;
  int decision;
  result_t(const char* s):subject(s),decision(AAA_NO_MATCH) {};
};

void result_callback(const std::string & attr,const std::string & value,void * ref) {
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

int AuthUser::match_ldap(const char* line) {
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
      olog<<"Connecting to "<<url.Host()<<":"<<url.Port()<<std::endl;
      olog<<"Quering at "<<url.Path()<<std::endl;
      std::vector<std::string> attrs; attrs.push_back("description");
      try {
        ldap.Query(url.Path(),"",attrs,gridftpd::LdapQuery::onelevel);
      } catch (gridftpd::LdapQueryError e) {
        olog<<"Failed to query ldap server "<<u<<std::endl;
        return AAA_FAILURE;
      };
      result_t r(subject.c_str());
      try {
        ldap.Result(&result_callback,&r) ;
      } catch (gridftpd::LdapQueryError e) {
        olog<<"Failed to get results from ldap server "<<u<<std::endl;
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
}
