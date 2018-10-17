#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>
#include <globus_gsi_credential.h>

#include <arc/FileUtils.h>
#include <arc/StringConv.h>
#include <arc/ArcConfigIni.h>

#include "../misc/proxy.h"

#include "auth.h"

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
  for(;;) {
    std::string s("");
    int n = Arc::ConfigIni::NextArg(line,s);
    if(n == 0) break;
    line+=n;
    for(std::list<group_t>::iterator i = groups.begin();i!=groups.end();++i) {
      if(s == i->name) {
        default_voms_=i->voms;
        default_vo_=i->vo;
        default_group_=i->name.c_str();
        return AAA_POSITIVE_MATCH;
      };
    };
  };
  return AAA_NO_MATCH;
}

AuthResult AuthUser::match_vo(const char* line) {
  for(;;) {
    std::string s("");
    int n = Arc::ConfigIni::NextArg(line,s);
    if(n == 0) break;
    line+=n;
    for(std::list<std::string>::iterator i = vos.begin();i!=vos.end();++i) {
      if(s == *i) {
        default_voms_=voms_t();
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
  { "userlist", &AuthUser::match_vo },
  { "plugin", &AuthUser::match_plugin },
  { NULL, NULL }
};


AuthUser::AuthUser(const char* s,const char* f):subject(""),filename("") {
  valid = true;
  if(s) { Arc::ConfigIni::NextArg(s,subject,'\0','\0'); };
  struct stat fileStat;
  if(f && stat(f, &fileStat) == 0) filename=f;
  proxy_file_was_created=false;
  voms_extracted=false;
  has_delegation=false; // ????
  default_voms_=voms_t();
  default_vo_=NULL;
  default_group_=NULL;
  if(process_voms() == AAA_FAILURE) valid=false;
}

AuthUser::AuthUser(const AuthUser& a) {
  valid=a.valid;
  subject=a.subject;
  filename=a.filename;
  has_delegation=a.has_delegation;
  proxy_file_was_created=false;
  voms_extracted=false;
  default_voms_=voms_t();
  default_vo_=NULL;
  default_group_=NULL;
  if(process_voms() == AAA_FAILURE) valid=false;
}

AuthUser& AuthUser::operator=(const AuthUser& a) {
  valid=a.valid;
  subject=a.subject;
  filename=a.filename;
  has_delegation=a.has_delegation;
  voms_data.clear();
  voms_extracted=false;
  proxy_file_was_created=false;
  default_voms_=voms_t();
  default_vo_=NULL;
  default_group_=NULL;
  if(process_voms() == AAA_FAILURE) valid=false;
  return *this;
}

void AuthUser::set(const char* s,gss_ctx_id_t ctx,gss_cred_id_t cred,const char* hostname) {
  valid=true;
  if(hostname) from=hostname;
  voms_data.clear();
  voms_extracted=false;
  proxy_file_was_created=false; filename=""; has_delegation=false;
  filename="";
  subject="";
  char* p = gridftpd::write_proxy(cred);
  if(p) {
    filename=p; free(p); has_delegation=true;
    proxy_file_was_created=true;
  } else {
    p=gridftpd::write_cert_chain(ctx);
    if(p) {
      filename=p; free(p);
      proxy_file_was_created=true;
    };
  };
  if(s == NULL) {
    // Obtain subject from credentials or context
    if(filename.length()) {
      globus_gsi_cred_handle_t h;
      if(globus_gsi_cred_handle_init(&h,GLOBUS_NULL) == GLOBUS_SUCCESS) {
        if(globus_gsi_cred_read_proxy(h,(char*)(filename.c_str())) == GLOBUS_SUCCESS) {
          char* sname = NULL;
          if(globus_gsi_cred_get_subject_name(h,&sname) == GLOBUS_SUCCESS) {
            Arc::ConfigIni::NextArg(sname,subject,'\0','\0'); free(sname);
          };
        };
        globus_gsi_cred_handle_destroy(h);
      };
    };
  } else {
    subject=s;
  };
  if(process_voms() == AAA_FAILURE) valid=false;
}

void AuthUser::set(const char* s,STACK_OF(X509)* cred,const char* hostname) {
  valid=true;
  if(hostname) from=hostname;
  voms_data.clear();
  voms_extracted=false;
  proxy_file_was_created=false; filename=""; has_delegation=false;
  int chain_size = 0;
  if(cred) chain_size=sk_X509_num(cred);
  if((s == NULL) && (chain_size <= 0)) return;
  if(s == NULL) {
    X509* cert=sk_X509_value(cred,0);
    if(cert) {
      X509_NAME *name = X509_get_subject_name(cert);
      if(name) {
        if(globus_gsi_cert_utils_get_base_name(name,cred) == GLOBUS_SUCCESS) {
          char* buf = X509_NAME_oneline(X509_get_subject_name(cert),NULL,0);
          if(buf) {
            subject=buf;
            OPENSSL_free(buf);
          };
        };
      };
    };
    if(subject.length() == 0) return;
  } else {
    subject=s;
  };
  if(chain_size > 0) {
    std::string tempname = Glib::build_filename(Glib::get_tmp_dir(), "x509.XXXXXX");
    if(!Arc::TmpFileCreate(tempname, "")) return;
    filename = tempname;
    BIO* bio;
    if((bio=BIO_new_file(filename.c_str(), "w")) == NULL) return;
    for(int chain_index = 0;chain_index<chain_size;++chain_index) {
      X509* cert=sk_X509_value(cred,chain_index);
      if(cert) {
        if(!PEM_write_bio_X509(bio,cert))  {
          BIO_free(bio); unlink(filename.c_str()); return;
        };  
      };
    };
    BIO_free(bio);
    proxy_file_was_created=true;
  };
  if(process_voms() == AAA_FAILURE) valid=false;
}

void AuthUser::set(const char* s,const char* hostname) {
  valid=true;
  if(hostname) from=hostname;
  voms_data.clear();
  voms_extracted=false;
  subject="";
  filename="";
  proxy_file_was_created=false; filename=""; has_delegation=false;
  if(s != NULL) subject=s;
  //if(process_voms() == AAA_FAILURE) valid=false;
}

struct voms_t AuthUser::arc_to_voms(const std::string& vo,const std::vector<std::string>& attributes) {

  struct voms_t voms_item;
  voms_item.voname = vo;
  // Collect fqans with parsed groups, roles and capabilties.
  for(std::vector<std::string>::const_iterator v = attributes.begin(); v != attributes.end(); ++v) {
    std::list<std::string> elements;
    Arc::tokenize(*v, elements, "/");
    // /rootgroup(=VO)/mygroup/mysubgroup/Role=myrole
    std::list<std::string>::iterator i = elements.begin();
    // Check root group agains VO and skip wrong ones
    if (i == elements.end()) continue; // too short
    if (*i != voms_item.voname) {
      // Check if that is VO to hostname association (special ARC FQAN)
      if(*i == (std::string("voname=")+voms_item.voname)) {
        ++i;
        if (*i != voms_item.voname) {
          std::vector<std::string> keyvalue;
          Arc::tokenize(*i, keyvalue, "=");
          if (keyvalue.size() == 2) {
            if (keyvalue[0] == "hostname") {
              voms_item.server = keyvalue[1];
            };
          };
        };
      };
      continue; // ignore attribute with wrong root group
    };
    voms_fqan_t fqan;
    fqan.group = "/"+(*i);
    ++i;
    for (; i != elements.end(); ++i) {
      std::vector<std::string> keyvalue;
      Arc::tokenize(*i, keyvalue, "=");
      if (keyvalue.size() == 1) { // part of group
        fqan.group += "/"+(*i);
      } else if (keyvalue.size() == 2) {
        if (keyvalue[0] == "Role") {
          fqan.role = keyvalue[1];
        } else if (keyvalue[0] == "Capability") {
          fqan.capability = keyvalue[1];
        }
      }
    }
    voms_item.fqans.push_back(fqan);
  }
  return voms_item;
}

AuthUser::~AuthUser(void) {
  if(proxy_file_was_created && filename.length()) unlink(filename.c_str());
}

AuthResult AuthUser::evaluate(const char* line) {
  if(!valid) return AAA_FAILURE; 
  bool invert = false;
  bool no_match = false;
  const char* command = "subject";
  size_t command_len = 7;
  if(subject.length()==0) return AAA_NO_MATCH; // ??
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
        };
      };
      return res;
    };
  };
  logger.msg(Arc::ERROR, "Unknown authorization command %s", command);
  return AAA_FAILURE; 
}

const std::vector<struct voms_t>& AuthUser::voms(void) {
  if(!voms_extracted) {
    const char* line = "* * * *";
    match_voms(line);
  };
  return voms_data;
}

const std::list<std::string>& AuthUser::VOs(void) const {
  return vos;
}

bool AuthUser::add_vo(const char* vo,const char* filename) {
  if((!filename) || (!filename[0])) {
    logger.msg(Arc::WARNING,"The [vo] section labeled '%s' has no file associated and can't be used for matching", vo);
    return false;
  }
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

std::string AuthUser::err_to_string(int err) {
  if(err == AAA_POSITIVE_MATCH) return "positive";
  if(err == AAA_NEGATIVE_MATCH) return "negative";
  if(err == AAA_NO_MATCH) return "no match";
  if(err == AAA_FAILURE) return "failure";
  return "";
}

AuthEvaluator::AuthEvaluator(void):name("") {

}

AuthEvaluator::AuthEvaluator(const char* s):name(s) {

}

AuthEvaluator::~AuthEvaluator(void) {

}

void AuthEvaluator::add(const char* line) {
  l.push_back(line);
}

AuthResult AuthEvaluator::evaluate(AuthUser &u) const {
  for(std::list<std::string>::const_iterator i = l.begin();i!=l.end();++i) {
    AuthResult r = u.evaluate(i->c_str());
    if(r != AAA_NO_MATCH) return r;
  };
  return AAA_NO_MATCH;
}

