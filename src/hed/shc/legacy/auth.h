#ifndef __GM_AUTH_H__
#define __GM_AUTH_H__

#include <string>
#include <list>
#include <vector>

#include <string.h>

#include <arc/message/Message.h>
//#include <gssapi.h>
//#include <openssl/x509.h>

namespace Arc {

#define AAA_POSITIVE_MATCH 1
#define AAA_NEGATIVE_MATCH -1
#define AAA_NO_MATCH 0
#define AAA_FAILURE 2

class AuthVO;

/** VOMS attributes */
struct voms_attrs {
  std::string group; /*!< user's group */
  std::string role;  /*!< user's role */
  std::string cap;   /*!< user's capability */
};

/** VOMS data */
struct voms {
  std::string server;      /*!< The VOMS server DN, as from its certificate */
  std::string voname;      /*!< The name of the VO to which the VOMS belongs */
  std::vector<voms_attrs> attrs;   /*!< User's characteristics */
};

class AuthUser {
 private:
  typedef int (AuthUser:: * match_func_t)(const char* line);
  typedef struct {
    const char* cmd;
    match_func_t func;
  } source_t;
  class group_t {
   public:
    const char* voms;             //
    std::string name;             //
    const char* vo;               //
    const char* role;             //
    const char* capability;       //
    const char* vgroup;           //
    group_t(const char* name_,const char* vo_,const char* role_,const char* cap_,const char* vgrp_,const char* voms_):voms(voms_?voms_:""),name(name_?name_:""),vo(vo_?vo_:""),role(role_?role_:""),capability(cap_?cap_:""),vgroup(vgrp_?vgrp_:"") { };
  };

  const char* default_voms_;
  const char* default_vo_;
  const char* default_role_;
  const char* default_capability_;
  const char* default_vgroup_;
  const char* default_group_;

  // Attributes of user
  std::string subject_;   // DN of certificate
  std::vector<struct voms> voms_data_; // VOMS information extracted from message

  // Old attributes - remove or convert
  std::string from;      // Remote hostname
  std::string filename;  // Delegated proxy stored in this file
  bool proxy_file_was_created; // If proxy file was created by this object
  bool has_delegation;   // If proxy contains delegation 

  // Matching methods
  static source_t sources[]; // Supported evaluation sources
  int match_all(const char* line);
  int match_group(const char* line);
  int match_subject(const char* line);
  int match_file(const char* line);
  int match_ldap(const char* line);
  int match_voms(const char* line);
  int match_vo(const char* line);
  int match_lcas(const char *);
  int match_plugin(const char* line);
  int process_voms(void);

//  bool voms_extracted;

  // Evaluation results
  std::list<group_t> groups; // Groups which user matched (internal names)
  std::list<std::string> vos; // VOs to which user belongs (external names)

  // References to related/source data
  Message& message_;

 public:
  AuthUser(const AuthUser&);
  // Constructor
  AuthUser(Message& message);

  // subject - subject/DN of user
  // filename - file with (delegated) credentials
  //AuthUser(const char* subject = NULL,const char* filename = NULL);

  ~AuthUser(void);
//  void operator=(const AuthUser&);
  // Reassign user with supplied credentials
  //void operator=(gss_cred_id_t cred);
  //void operator=(gss_ctx_id_t ctx);
  //void set(const char* subject,const char* hostname = NULL);
//  void set(const char* subject,gss_ctx_id_t ctx,gss_cred_id_t cred,const char* hostname = NULL);
  //void set(const char* s,STACK_OF(X509)* cred,const char* hostname = NULL);
  // Evaluate authentication rules
  int evaluate(const char* line);
  const char* DN(void) const { return subject_.c_str(); };
  const char* proxy(void) const { return filename.c_str(); };
  bool is_proxy(void) const { return has_delegation; };
  const char* hostname(void) const { return from.c_str(); };
  // Remember this user belongs to group 'grp'
  void add_group(const char* grp) {
    groups.push_back(group_t(grp,default_vo_,default_role_,default_capability_,default_vgroup_,default_voms_));
  };
  void add_group(const std::string& grp) { add_group(grp.c_str()); };
  // Mark this user as belonging to no no groups
  void clear_groups(void) { groups.clear(); default_group_=NULL; };
  // Returns true if user belongs to specified group 'grp'
  bool check_group(const char* grp) const {
    for(std::list<group_t>::const_iterator i=groups.begin();i!=groups.end();++i) {
      if(strcmp(i->name.c_str(),grp) == 0) return true;
    };
    return false;
  };
  bool check_group(const std::string& grp) const { return check_group(grp.c_str());};
  void add_vo(const char* vo) { vos.push_back(std::string(vo)); };
  void add_vo(const std::string& vo) { vos.push_back(vo); };
  bool add_vo(const char* vo,const char* filename);
  bool add_vo(const std::string& vo,const std::string& filename);
  bool add_vo(const AuthVO& vo);
  bool add_vo(const std::list<AuthVO>& vos);
  void clear_vos(void) { vos.clear(); };
  bool check_vo(const char* vo) const {
    for(std::list<std::string>::const_iterator i=vos.begin();i!=vos.end();++i) {
      if(strcmp(i->c_str(),vo) == 0) return true;
    };
    return false;
  };
  bool check_vo(const std::string& vo) const { return check_vo(vo.c_str());};
  //const char* default_voms(void) const { return default_voms_; };
  //const char* default_vo(void) const { return default_vo_; };
  //const char* default_role(void) const { return default_role_; };
  //const char* default_capability(void) const { return default_capability_; };
  //const char* default_vgroup(void) const { return default_vgroup_; };
  //const char* default_group(void) const { return default_group_; };
  //const char* default_subject(void) const { return subject.c_str(); };
  const std::vector<struct voms>& voms(void);
  const std::list<std::string>& VOs(void);
  // convert ARC list into voms structure
  static std::vector<struct voms> arc_to_voms(const std::vector<std::string>& attributes);
  /*
   * Get a certain property of the AuthUser, for example DN
   * or VOMS VO. For possible values of property see the source
   * code in auth.cc
   *
   * Not used in gridftpd
   */
  const std::string get_property(const std::string /* property */) {
    return std::string("");
  };
};

class AuthEvaluator {
 private:
  std::list<std::string> l;
  std::string name;
 public:
  AuthEvaluator(void);
  AuthEvaluator(const char* name);
  ~AuthEvaluator(void);
  void add(const char*);
  int evaluate(AuthUser &) const;
  bool operator==(const char* s) { return (strcmp(name.c_str(),s)==0); };
  bool operator==(const std::string& s) { return (name == s); };
  const char* get_name() { return name.c_str(); };
};

void AuthUserSubst(std::string& str,AuthUser& it);

class AuthVO {
 friend class AuthUser;
 private:
  std::string name;
  std::string file;
 public:
  AuthVO(const char* vo,const char* filename):name(vo),file(filename) { };
  AuthVO(const std::string& vo,const std::string& filename):name(vo.c_str()),file(filename.c_str()) { };
  ~AuthVO(void) { };
};

} // namespace Arc

#endif 
