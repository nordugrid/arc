#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>
#include <fstream>
#include <string>
#include <list>

#include <arc/Logger.h>
#include <arc/ArcRegex.h>

#include <openssl/x509.h>

#include "GlobusSigningPolicy.h"

namespace Arc {

static Logger& logger = Logger::getRootLogger();

const char access_id[]       = "access_id_";
const char positive_rights[] = "pos_rights";
const char negative_rights[] = "neg_rights";
const char globus_id[]       = "globus";
const char sign_id[]         = "CA:sign";
const char conditions_id[]   = "cond_";
const char policy_suffix[]   = ".signing_policy";

static void get_line(std::istream& in,std::string& s) {
  for(;;) {
    s.resize(0);
    if(in.fail() || in.eof()) return;
    getline(in,s);
    std::string::size_type p;
    p=s.find_first_not_of(" \t");
    if(p != std::string::npos) s=s.substr(p);
    if((!s.empty()) && (s[0] != '#')) break;
  };
  return;
}

static void get_word(std::string& s,std::string& word) {
  std::string::size_type w_s;
  std::string::size_type w_e;
  word.resize(0);
  w_s=s.find_first_not_of(" \t");
  if(w_s == std::string::npos) { s.resize(0); return; };
  if(s[w_s] == '\'') {
    w_e=s.find('\'',++w_s);
  } else if(s[w_s] == '"') {
    w_e=s.find('"',++w_s);
  } else {
    w_e=s.find_first_of(" \t",w_s);
  };
  if(w_e == std::string::npos) w_e=s.length();
  word=s.substr(w_s,w_e-w_s);
  if((s[w_e] == '\'') || (s[w_e] == '"')) ++w_e;
  w_s=s.find_first_not_of(" \t",w_e);
  if(w_s == std::string::npos) w_s=w_e;
  s=s.substr(w_s);
  return;
}

static bool get_id(std::string& s,std::string& ca_subject) {
  std::string id;
  ca_subject.resize(0);
  get_word(s,id);
  if(id.empty()) return true;
  if(id.compare(0,strlen(access_id),access_id) != 0) {
    logger.msg(Arc::WARNING,"Was expecting %s at the beginning of \"%s\"",access_id,id);
    return false;
  };
  id=id.substr(strlen(access_id));
  if(id != "CA") { 
    logger.msg(Arc::WARNING,"We only support CAs in Globus signing policy - %s is not supported",id);
    return false;
  };
  get_word(s,id);
  if(id != "X509") {
    logger.msg(Arc::WARNING,"We only support X509 CAs in Globus signing policy - %s is not supported",id);
    return false;
  };
  get_word(s,ca_subject);
  if(ca_subject.empty()) {
    logger.msg(Arc::WARNING,"Missing CA subject in Globus signing policy");
    return false;
  };
  return true;
}

static bool get_rights(std::string& s) {
  std::string id;
  get_word(s,id);
  if(id == negative_rights) {
    logger.msg(Arc::WARNING,"Negative rights are not supported in Globus signing policy");
    return false;
  };
  if(id != positive_rights) {
    logger.msg(Arc::WARNING,"Unknown rights in Globus signing policy - %s",id);
    return false;
  };
  get_word(s,id);
  if(id != globus_id) {
    logger.msg(Arc::WARNING,"Only globus rights are supported in Globus signing policy - %s is not supported",id);
    return false;
  };
  get_word(s,id);
  if(id != sign_id) {
    logger.msg(Arc::WARNING,"Only signing rights are supported in Globus signing policy - %s is not supported",id);
    return false;
  };
  return true;
}

static bool get_conditions(std::string s,std::list<std::string>& patterns) {
  std::string id;
  patterns.resize(0);
  get_word(s,id);
  if(id.empty()) return true;
  if(id.compare(0,strlen(conditions_id),conditions_id) != 0) {
    logger.msg(Arc::WARNING,"Was expecting %s at the beginning of \"%s\"",conditions_id,id);
    return false;
  };
  id=id.substr(strlen(conditions_id));
  if(id != "subjects") {
    logger.msg(Arc::WARNING,"We only support subjects conditions in Globus signing policy - %s is not supported",id);
    return false;
  };
  get_word(s,id);
  if(id != globus_id) {
    logger.msg(Arc::WARNING,"We only support globus conditions in Globus signing policy - %s is not supported",id);
    return false;
  };
  std::string subjects;
  get_word(s,subjects);
  if(subjects.empty()) {
    logger.msg(Arc::WARNING,"Missing condition subjects in Globus signing policy");
    return false;
  };
  std::string subject;
  for(;;) {
    get_word(subjects,subject);
    if(subject.empty()) break;
    patterns.push_back(subject);
  };
  return true;
}

static bool match_all(const std::string& issuer_subject,const std::string& subject,const std::string policy_ca_subject,std::list<std::string> policy_patterns) {
  if(issuer_subject == policy_ca_subject) {
    std::list<std::string>::iterator pattern = policy_patterns.begin();
    for(;pattern!=policy_patterns.end();++pattern) {
      std::string::size_type p = 0;
      for(;;) {
        p=pattern->find('*',p);
        if(p == std::string::npos) break;
        pattern->insert(p,"."); p+=2;
      };
      (*pattern)="^"+(*pattern)+"$";
      RegularExpression re(*pattern);
      bool r = re.match(subject);
      if(r) return true;
    };
  };
  return false;
}

static void X509_NAME_to_string(std::string& str,const X509_NAME* name) {
  str.resize(0);
  if(name == NULL) return;
  char* s = X509_NAME_oneline((X509_NAME*)name,NULL,0);
  if(s) {
    str=s;
    OPENSSL_free(s);
  };
  return;
}

bool match_globus_policy(std::istream& in,const X509_NAME* issuer_subject,const X509_NAME* subject) {
  std::string issuer_subject_str;
  std::string subject_str;
  std::string s;
  std::string policy_ca_subject;
  std::list<std::string> policy_patterns;
  bool rights_defined = false;
  bool failure = false;
  X509_NAME_to_string(issuer_subject_str,issuer_subject);
  X509_NAME_to_string(subject_str,subject);
  for(;;) {
    get_line(in,s);
    if(s.empty()) break;
    if(s.compare(0,strlen(access_id),access_id) == 0) {
      if((!policy_ca_subject.empty()) && (rights_defined) && (!failure)) {
        bool r = match_all(issuer_subject_str,subject_str,policy_ca_subject,policy_patterns);
        if(r) return true;
      };
      policy_ca_subject.resize(0);
      policy_patterns.resize(0);
      failure=false; rights_defined=false;
      if(!get_id(s,policy_ca_subject)) failure=true;
    } else if((s.compare(0,strlen(positive_rights),positive_rights) == 0) ||
              (s.compare(0,strlen(positive_rights),negative_rights) == 0)) {
      if(!get_rights(s)) {
        failure=true;
      } else {
        rights_defined=true;
      };
    } else if(s.compare(0,strlen(conditions_id),conditions_id) == 0) {
      if(!get_conditions(s,policy_patterns)) failure=true;
    } else {
      logger.msg(Arc::WARNING,"Unknown element in Globus signing policy");
      failure=true;
    };
  };
  if((!policy_ca_subject.empty()) && (rights_defined) && (!failure)) {
    bool r = match_all(issuer_subject_str,subject_str,policy_ca_subject,policy_patterns);
    if(r) return true;
  };
  return false;
}

std::istream* open_globus_policy(const X509_NAME* issuer_subject,const std::string& ca_path) {
  std::string issuer_subject_str;
  X509_NAME_to_string(issuer_subject_str,issuer_subject);
  unsigned long hash = X509_NAME_hash((X509_NAME*)issuer_subject);
  char hash_str[32];
  snprintf(hash_str,sizeof(hash_str)-1,"%08lx",hash);
  hash_str[sizeof(hash_str)-1]=0;
  std::string fname = ca_path+"/"+hash_str+policy_suffix;
  std::ifstream* f = new std::ifstream(fname.c_str());
  if(!(*f)) { delete f; return NULL; };
  return f;
}

}

