#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

//#include <iostream>
//#include <globus_gsi_credential.h>

#include <arc/StringConv.h>
//#include <arc/Utils.h>

//#include "../misc/proxy.h"
//#include "../misc/escaped.h"

#include "LegacySecAttr.h"

#include "auth.h"

namespace ArcSHCLegacy {

int AuthUser::match_all(const char* /* line */) {
  default_voms_=NULL;
  default_vo_=NULL;
  default_role_=NULL;
  default_capability_=NULL;
  default_vgroup_=NULL;
  default_group_=NULL;
  return AAA_POSITIVE_MATCH;
}

int AuthUser::match_group(const char* line) {
  std::string::size_type n = 0;
  for(;;) {
    if(n == std::string::npos) break;
    std::string s("");
    n = Arc::get_token(s,line,n," ","\"","\"");
    if(s.empty()) continue;
    for(std::list<group_t>::iterator i = groups_.begin();i!=groups_.end();++i) {
      if(s == i->name) {
        default_voms_=i->voms;
        default_vo_=i->vo;
        default_role_=i->role;
        default_capability_=i->capability;
        default_vgroup_=i->vgroup;
        default_group_=i->name.c_str();
        return AAA_POSITIVE_MATCH;
      };
    };
  };
  return AAA_NO_MATCH;
}

int AuthUser::match_vo(const char* line) {
  std::string::size_type n = 0;
  for(;;) {
    std::string s("");
    n = Arc::get_token(s,line,n," ","\"","\"");
    if(s.empty()) continue;
    for(std::list<std::string>::iterator i = vos_.begin();i!=vos_.end();++i) {
      if(s == *i) {
        default_voms_=NULL;
        default_vo_=i->c_str();
        default_role_=NULL;
        default_capability_=NULL;
        default_vgroup_=NULL;
        default_group_=NULL;
        return AAA_POSITIVE_MATCH;
      };
    };
  };
  return AAA_NO_MATCH;
}

AuthUser::source_t AuthUser::sources[] = {
  { "all", &AuthUser::match_all },
  { "group", &AuthUser::match_group },
  { "subject", &AuthUser::match_subject },
  { "file", &AuthUser::match_file },
  { "remote", &AuthUser::match_ldap },
  { "voms", &AuthUser::match_voms },
  { "vo", &AuthUser::match_vo },
  { "lcas", &AuthUser::match_lcas },
  { "plugin", &AuthUser::match_plugin },
  { NULL, NULL }
};

AuthUser::AuthUser(const AuthUser& a):message_(a.message_) {
  subject_ = a.subject_;
  voms_data_ = a.voms_data_;

  filename=a.filename;
  has_delegation=a.has_delegation;
  proxy_file_was_created=false;
//  process_voms();
  default_voms_=NULL;
  default_vo_=NULL;
  default_role_=NULL;
  default_capability_=NULL;
  default_vgroup_=NULL;
  default_group_=NULL;
}

AuthUser::AuthUser(Arc::Message& message):message_(message) {
  subject_ = message.Attributes()->get("TLS:IDENTITYDN");
  // Fetch VOMS attributes
  std::vector<std::string> voms_attrs;
  Arc::XMLNode tls_attrs;
  std::list<std::string> auth_keys_allow;
  std::list<std::string> auth_keys_reject;
  auth_keys_allow.push_back("TLS");
  Arc::MessageAuth* auth = NULL;
  auth = message.Auth()->Filter(auth_keys_allow,auth_keys_reject);
  if(auth) {
    auth->Export(Arc::SecAttr::ARCAuth,tls_attrs);
    delete auth; auth = NULL;
  };
  auth = message.AuthContext()->Filter(auth_keys_allow,auth_keys_reject);
  if(auth) {
    auth->Export(Arc::SecAttr::ARCAuth,tls_attrs);
    delete auth; auth = NULL;
  };
  Arc::XMLNode attr = tls_attrs["RequestItem"]["Subject"]["SubjectAttribute"];
  for(;(bool)attr;++attr) {
    if((std::string)tls_attrs.Attribute("AttributeId") == "http://www.nordugrid.org/schemas/policy-arc/types/tls/vomsattribute") {
      voms_attrs.push_back((std::string)attr);
    };
  };
  voms_data_ = arc_to_voms(voms_attrs);
}

std::vector<struct voms> AuthUser::arc_to_voms(const std::vector<std::string>& attributes) {

  std::vector<struct voms> voms_list;
  struct voms voms_item;
  for(std::vector<std::string>::const_iterator v = attributes.begin(); v != attributes.end(); ++v) {
    struct voms_attrs attrs;
    std::string vo;
    std::string attr = v->substr(v->find(" ")+1);
    std::list<std::string> elements;
    Arc::tokenize(*v, elements, "/");
    for (std::list<std::string>::iterator i = elements.begin(); i != elements.end(); ++i) {
      std::vector<std::string> keyvalue;
      Arc::tokenize(*i, keyvalue, "=");
      if (keyvalue.size() == 2) {
        if (keyvalue[0] == "voname") { // new VO
          if (v != attributes.begin() && keyvalue[1] != voms_item.voname) {
            voms_list.push_back(voms_item);
            voms_item.attrs.clear();
          }
          voms_item.voname = keyvalue[1];
        }
        else if (keyvalue[0] == "hostname")
          voms_item.server = keyvalue[1];
        else {
          // /VO=myvo/Group=mygroup/Role=myrole
          if (keyvalue[0] == "VO")
            vo = keyvalue[1];
          else if (keyvalue[0] == "Role")
            attrs.role = keyvalue[1];
          else if (keyvalue[0] == "Group")
            attrs.group = keyvalue[1];
          else if (keyvalue[0] == "Capability")
            attrs.cap = keyvalue[1];
        }
      }
    }
    if (!vo.empty() || !attrs.cap.empty() || !attrs.group.empty() || !attrs.role.empty())
      voms_item.attrs.push_back(attrs);
  }
  voms_list.push_back(voms_item);
  return voms_list;
}

AuthUser::~AuthUser(void) {
  if(proxy_file_was_created && filename.length()) unlink(filename.c_str());
}

int AuthUser::evaluate(const char* line) {
  bool invert = false;
  bool no_match = false;
  const char* command = "subject";
  size_t command_len = 7;
  if(subject_.empty()) return AAA_NO_MATCH;
  if(!line) return AAA_NO_MATCH;
  for(;*line;line++) if(!isspace(*line)) break;
  if(*line == 0) return AAA_NO_MATCH;
  if(*line == '#') return AAA_NO_MATCH;
  if(*line == '-') { line++; invert=true; }
  else if(*line == '+') { line++; };
  if(*line == '!') { no_match=true; line++; };
  if((*line != '/') && (*line != '"')) {
    command=line; 
    for(;*line;line++) if(isspace(*line)) break;
    command_len=line-command;
    for(;*line;line++) if(!isspace(*line)) break;
  };
  for(source_t* s = sources;s->cmd;s++) {
    if((strncmp(s->cmd,command,command_len) == 0) && 
       (strlen(s->cmd) == command_len)) {
      int res=(this->*(s->func))(line);
      if(res == AAA_FAILURE) return res;
      if(no_match) {
        if(res==AAA_NO_MATCH) { res=AAA_POSITIVE_MATCH; }
        else { res=AAA_NO_MATCH; };
      };
      if(invert) res=-res;
      return res;
    };
  };
  return AAA_FAILURE; 
}

const std::list<std::string>& AuthUser::VOs(void) {
  return vos_;
}

bool AuthUser::add_vo(const char* vo,const char* filename) {
  if(match_file(filename) == AAA_POSITIVE_MATCH) {
    add_vo(vo);
    return true;
  };
  return false;
}

bool AuthUser::add_vo(const std::string& vo,const std::string& filename) {
  return add_vo(vo.c_str(),filename.c_str());
}

bool AuthUser::add_vo(const AuthVO& vo) {
  return add_vo(vo.name,vo.file);
}

bool AuthUser::add_vo(const std::list<AuthVO>& vos) {
  bool r = true;
  for(std::list<AuthVO>::const_iterator vo = vos.begin();vo!=vos.end();++vo) {
    r&=add_vo(*vo);
  };
  return r;
}

void AuthUser::get_groups(std::list<std::string>& groups) const {
  for(std::list<group_t>::const_iterator g = groups_.begin();g!=groups_.end();++g) {
    groups.push_back(g->name);
  };
}

} // namespace ArcSHCLegacy

