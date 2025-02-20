#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/StringConv.h>
#include <arc/FileUtils.h>
#include <arc/message/SecAttr.h>

#include "LegacySecAttr.h"

#include "auth.h"

namespace ArcSHCLegacy {

static Arc::Logger logger(Arc::Logger::getRootLogger(),"AuthUser");

void voms_fqan_t::str(std::string& str) const {
  str = group;
  if(!role.empty()) str += "/Role="+role;
  if(!capability.empty()) str += "/Capability="+capability;
}

AuthResult AuthUser::match_all(const char* line) {
  std::string token = Arc::trim(line);
  if(token == "yes") {
    default_voms_=voms_t();
    default_otokens_=otokens_t();
    default_vo_=NULL;
    default_group_=NULL;
    return AAA_POSITIVE_MATCH;
  }
  if(token == "no") {
    return AAA_NO_MATCH;
  }
  logger.msg(Arc::ERROR,"Unexpected argument for 'all' rule - %s",token);
  return AAA_FAILURE;
}

AuthResult AuthUser::match_group(const char* line) {
  std::string::size_type n = 0;
  for(;;) {
    if(n == std::string::npos) break;
    std::string s("");
    n = Arc::get_token(s,line,n," ");
    if(s.empty()) continue;
    for(std::list<group_t>::iterator i = groups_.begin();i!=groups_.end();++i) {
      if(s == i->name) {
        default_voms_=voms_t();
        default_otokens_=otokens_t();
        default_vo_=i->vo;
        default_group_=i->name.c_str();
        return AAA_POSITIVE_MATCH;
      };
    };
  };
  return AAA_NO_MATCH;
}

AuthResult AuthUser::match_vo(const char* line) {
  std::string::size_type n = 0;
  for(;;) {
    if(n == std::string::npos) break;
    std::string s("");
    n = Arc::get_token(s,line,n," ");
    if(s.empty()) continue;
    for(std::list<std::string>::iterator i = vos_.begin();i!=vos_.end();++i) {
      if(s == *i) {
        default_voms_=voms_t();
        default_otokens_=otokens_t();
        default_vo_=i->c_str();
        default_group_=NULL;
        return AAA_POSITIVE_MATCH;
      };
    };
  };
  return AAA_NO_MATCH;
}

AuthUser::source_t AuthUser::sources[] = {
  { "all", &AuthUser::match_all },
  { "authgroup", &AuthUser::match_group },
  { "subject", &AuthUser::match_subject },
  { "file", &AuthUser::match_file },
  { "voms", &AuthUser::match_voms },
  { "authtokens", &AuthUser::match_otokens },
  { "authtokensgen", &AuthUser::match_ftokens },
  { "userlist", &AuthUser::match_vo },
  { "plugin", &AuthUser::match_plugin },
  { NULL, NULL }
};

AuthUser::AuthUser(const AuthUser& a):message_(a.message_) {
  subject_ = a.subject_;
  voms_data_ = a.voms_data_;
  otokens_data_ = a.otokens_data_;
  from = a.from;

  filename=a.filename;
  has_delegation=a.has_delegation;
  proxy_file_was_created=false;
//  process_voms();
  default_voms_=voms_t();
  default_otokens_=otokens_t();
  default_vo_=NULL;
  default_group_=NULL;

  groups_ = a.groups_;
  vos_ = a.vos_;
}

AuthUser::AuthUser(Arc::Message& message):
    default_voms_(), default_otokens_(), default_vo_(NULL), default_group_(NULL),
    proxy_file_was_created(false), has_delegation(false), message_(message) {
  // Fetch X.509 and VOMS attributes
  std::list<std::string> voms_attrs;
  Arc::SecAttr* sattr = NULL;
  sattr = message_.Auth()->get("TLS");
  if(sattr) {
    subject_ = sattr->get("IDENTITY");
    std::list<std::string> vomses = sattr->getAll("VOMS");
    voms_attrs.splice(voms_attrs.end(),vomses);
  };
  sattr = message_.AuthContext()->get("TLS");
  if(sattr) {
    if(subject_.empty())
      subject_ = sattr->get("IDENTITY");
    std::list<std::string> vomses = sattr->getAll("VOMS");
    voms_attrs.splice(voms_attrs.end(),vomses);
  };
  voms_data_ = arc_to_voms(voms_attrs);

  // Fetch OTokens attributes
  sattr = message_.Auth()->get("OTOKENS");
  if(sattr) {
    otokens_t otokens;
    otokens.subject  = sattr->get("sub");
    otokens.issuer   = sattr->get("iss");
    otokens.audiences= sattr->getAll("aud");
    Arc::tokenize(sattr->get("scope"), otokens.scopes);
    otokens.groups   = sattr->getAll("wlcg.groups");
    otokens.claims   = sattr->getAll();
    otokens_data_.push_back(otokens);
    if(subject_.empty())
      subject_ = sattr->get("iss+sub");
  };
  sattr = message_.AuthContext()->get("OTOKENS");
  if(sattr) {
    otokens_t otokens;
    otokens.subject  = sattr->get("sub");
    otokens.issuer   = sattr->get("iss");
    otokens.audiences= sattr->getAll("aud");
    Arc::tokenize(sattr->get("scope"), otokens.scopes);
    otokens.groups   = sattr->getAll("wlcg.groups");
    otokens.claims   = sattr->getAll();
    otokens_data_.push_back(otokens);
    if(subject_.empty())
      subject_ = sattr->get("iss+sub");
  };
}

// The attributes passed to this method are of "extended fqan" kind with every field 
// made of key=value pair. Also each attribute has /VO=voname prepended.
// Special ARC attribute /voname=voname/hostname=hostname is used for assigning 
// server host name to VO.
std::vector<struct voms_t> AuthUser::arc_to_voms(const std::list<std::string>& attributes) {

  std::vector<struct voms_t> voms_list;
  struct voms_t voms_item;

  for(std::list<std::string>::const_iterator v = attributes.begin(); v != attributes.end(); ++v) {
    std::list<std::string> elements;
    Arc::tokenize(*v, elements, "/");
    // Handle first element which contains VO name
    std::list<std::string>::iterator i = elements.begin();
    if(i == elements.end()) continue; // empty attribute?
    // Handle first element which contains VO name
    std::vector<std::string> keyvalue;
    Arc::tokenize(*i, keyvalue, "=");
    if (keyvalue.size() != 2) continue; // improper record
    if (keyvalue[0] == "voname") { // VO to hostname association
      if(voms_item.voname != keyvalue[1]) {
        if(!voms_item.voname.empty()) {
          voms_list.push_back(voms_item);
        };
        voms_item = voms_t();
        voms_item.voname = keyvalue[1];
      };
      ++i;
      if(i != elements.end()) {
        Arc::tokenize(*i, keyvalue, "=");
        if (keyvalue.size() == 2) {
          if (keyvalue[0] == "hostname") {
            voms_item.server = keyvalue[1];
          };
        };
      };
      continue;
    } else if(keyvalue[0] == "VO") {
      if(voms_item.voname != keyvalue[1]) {
        if(!voms_item.voname.empty()) {
          voms_list.push_back(voms_item);
        };
        voms_item = voms_t();
        voms_item.voname = keyvalue[1];
      };
    } else {
      // Skip unknown record
      continue;
    }
    ++i;
    voms_fqan_t fqan;
    for (; i != elements.end(); ++i) {
      std::vector<std::string> keyvalue;
      Arc::tokenize(*i, keyvalue, "=");
      // /Group=mygroup/Role=myrole
      // Ignoring unrecognized records
      if (keyvalue.size() == 2) {
        if (keyvalue[0] == "Group") {
          fqan.group += "/"+keyvalue[1];
        } else if (keyvalue[0] == "Role") {
          fqan.role = keyvalue[1];
        } else if (keyvalue[0] == "Capability") {
          fqan.capability = keyvalue[1];
        };
      };
    };
    voms_item.fqans.push_back(fqan);
  };
  if(!voms_item.voname.empty()) {
    voms_list.push_back(voms_item);
  };
  return voms_list;
}

AuthUser::~AuthUser(void) {
  if(filename.length()) Arc::FileDelete(filename);
}

AuthResult AuthUser::evaluate(const char* line) {
  bool invert = false;
  bool no_match = false;
  const char* command = "subject";
  size_t command_len = 7;
  // There can be rules not based on subject
  // if(subject_.empty()) return AAA_NO_MATCH; // ??
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
      AuthResult res=(this->*(s->func))(line);
      if(res == AAA_FAILURE) return res;
      if(no_match) {
        if(res==AAA_NO_MATCH) { res=AAA_POSITIVE_MATCH; }
        else { res=AAA_NO_MATCH; };
      };
      if(invert) {
        switch(res) {
          case AAA_POSITIVE_MATCH: res = AAA_NEGATIVE_MATCH; break;
          case AAA_NEGATIVE_MATCH: res = AAA_POSITIVE_MATCH; break;
          case AAA_NO_MATCH:
          case AAA_FAILURE:
          default:
            break;
        };
      };
      return res;
    };
  };
  return AAA_FAILURE; 
}

const std::list<std::string>& AuthUser::VOs(void) {
  return vos_;
}

void AuthUser::get_groups(std::list<std::string>& groups) const {
  for(std::list<group_t>::const_iterator g = groups_.begin();g!=groups_.end();++g) {
    groups.push_back(g->name);
  };
}

void AuthUser::subst(std::string& str) {
  int l = str.length();
  // Substitutions: %D, %P
  for(int i=0;i<l;i++) {
    if(str[i] == '%') {
      if(i<(l-1)) {
        switch(str[i+1]) {
          case 'D': {
            const char* s = subject();
            int s_l = strlen(s);
            str.replace(i,2,s);
            i+=(s_l-2-1);
          }; break;
          case 'P': {
            const char* s = proxy();
            int s_l = strlen(s);
            str.replace(i,2,s);
            i+=(s_l-2-1);
          }; break;
          default: {
            i++;
          }; break;
        };
      };
    };
  };
}

bool AuthUser::store_credentials(void) {
  if(!filename.empty()) return true;
  Arc::SecAttr* sattr = message_.Auth()->get("TLS");
  std::string cred;
  if(sattr) {
    cred = sattr->get("CERTIFICATE");
  };
  if(cred.empty()) {
    sattr = message_.AuthContext()->get("TLS");
    if(sattr) {
      cred = sattr->get("CERTIFICATE");
    };
  };
  if(!cred.empty()) {
    cred+=sattr->get("CERTIFICATECHAIN");
    std::string tmpfile;
    if(Arc::TmpFileCreate(tmpfile,cred)) {
      filename = tmpfile;
      logger.msg(Arc::VERBOSE,"Credentials stored in temporary file %s",filename);
      return true;
    };
  };
  return false;
}

void AuthUser::add_group(const std::string& grp) {
  groups_.push_back(group_t(grp,default_vo_,default_voms_,default_otokens_));
  logger.msg(Arc::VERBOSE,"Assigned to authorization group %s",grp);
};

void AuthUser::add_vo(const std::string& vo) {
  vos_.push_back(vo);
  logger.msg(Arc::VERBOSE,"Assigned to userlist %s",vo);
}

void AuthUser::add_groups(const std::list<std::string>& grps) {
  for(std::list<std::string>::const_iterator grp = grps.begin();
                                    grp != grps.end(); ++grp) {
    add_group(*grp);
  }
}

void AuthUser::add_vos(const std::list<std::string>& vos) {
  for(std::list<std::string>::const_iterator vo = vos.begin();
                                    vo != vos.end(); ++vo) {
    add_vo(*vo);
  }
}


} // namespace ArcSHCLegacy

