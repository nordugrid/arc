#ifndef __GM_AUTH_H__
#define __GM_AUTH_H__

#include <string>
#include <list>
#include <vector>

#include <gssapi.h>
#include <openssl/x509.h>

enum AuthResult {
  AAA_POSITIVE_MATCH = 1,
  AAA_NEGATIVE_MATCH = -1,
  AAA_NO_MATCH = 0,
  AAA_FAILURE = 2
};

class AuthVO;

/** VOMS FQAN split into elements */
struct voms_fqan_t {
  std::string group;      // including root group which is always same as VO
  std::string role;       // role associated to group - for each role there is one voms_fqan_t
  std::string capability; // deprecated but must keep itt
  void str(std::string& str) const; // convert to string (minimal variation)
};

/** VOMS data */
struct voms_t {
  std::string server;      /*!< The VOMS server hostname */
  std::string voname;      /*!< The name of the VO to which the VOMS belongs */
  std::vector<voms_fqan_t> fqans; /*!< Processed FQANs of user */
};

class AuthUser {
 private:
  typedef AuthResult (AuthUser:: * match_func_t)(const char* line);
  typedef struct {
    const char* cmd;
    match_func_t func;
  } source_t;
  class group_t {
   public:
    std::string name;             //
    const char* vo;               // VO name matched when authorizing this group
    struct voms_t voms;           // VOMS attributes matched when authorizing this group
    group_t(const char* name_, const char* vo_, const struct voms_t& voms_):
        name(name_?name_:""),vo(vo_?vo_:""),voms(voms_) { };
  };
  // VOMS attributes which matched last athorization rule. Also affected by matching group.
  struct voms_t default_voms_; 
  // Last matched VO name from those defined in [vo].
  const char* default_vo_;
  // Last matched group including groupcfg processing.
  const char* default_group_;
  std::string subject;   // SN of certificate
  std::string from;      // Remote hostname
  std::string filename;  // Delegated proxy stored in this file
  bool proxy_file_was_created; // If proxy file was created by this object
  bool has_delegation;   // If proxy contains delegation 
  static source_t sources[]; // Supported evaluation sources
  AuthResult match_all(const char* line);
  AuthResult match_group(const char* line);
  AuthResult match_subject(const char* line);
  AuthResult match_file(const char* line);
  AuthResult match_ldap(const char* line);
  AuthResult match_voms(const char* line);
  AuthResult match_vo(const char* line);
  AuthResult match_lcas(const char *);
  AuthResult match_plugin(const char* line);
  AuthResult process_voms(void);
  std::vector<struct voms_t> voms_data; // VOMS information extracted from proxy
  bool voms_extracted;
  std::list<group_t> groups; // Groups which user matched (internal names)
  std::list<std::string> vos; // VOs to which user belongs (external names)
  bool valid;
  const group_t* find_group(const char* grp) const {
    if(grp == NULL) return NULL;
    for(std::list<group_t>::const_iterator i=groups.begin();i!=groups.end();++i) {
      if(i->name == grp) return &(*i);
    };
    return NULL;
  };
  const group_t* find_group(const std::string& grp) const { return find_group(grp.c_str());};
 public:
  AuthUser(const AuthUser&);
  // Constructor
  // subject - subject/DN of user
  // filename - file with (delegated) credentials
  AuthUser(const char* subject = NULL,const char* filename = NULL);
  ~AuthUser(void);
  AuthUser& operator=(const AuthUser&);
  bool operator!(void) { return !valid; };
  operator bool(void) { return valid; };
  void set(const char* subject,const char* hostname = NULL);
  void set(const char* subject,gss_ctx_id_t ctx,gss_cred_id_t cred,const char* hostname = NULL);
  void set(const char* s,STACK_OF(X509)* cred,const char* hostname = NULL);
  // Evaluate authentication rules
  AuthResult evaluate(const char* line);
  const char* DN(void) const { return subject.c_str(); };
  const char* proxy(void) const { return filename.c_str(); };
  bool is_proxy(void) const { return has_delegation; };
  const char* hostname(void) const { return from.c_str(); };
  // Remember this user belongs to group 'grp'
  void add_group(const char* grp) {
    groups.push_back(group_t(grp,default_vo_,default_voms_));
  };
  void add_group(const std::string& grp) { add_group(grp.c_str()); };
  // Mark this user as belonging to no groups
  void clear_groups(void) { groups.clear(); default_group_=NULL; };
  // Returns true if user belongs to specified group 'grp'
  bool check_group(const char* grp) const;
  bool check_group(const std::string& grp) const { return check_group(grp.c_str());};
  bool select_group(const char* grp);
  bool select_group(const std::string& grp) { return select_group(grp.c_str());};
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
  const struct voms_t& default_voms(void) const { return default_voms_; };
  const char* default_vo(void) const { return default_vo_; };
  const char* default_group(void) const { return default_group_; };
  const struct voms_t* default_group_voms(void) const {
    const group_t* group = find_group(default_group_);
    return (group == NULL)?NULL:&(group->voms);
  };
  const char* default_group_vo(void) const {
    const group_t* group = find_group(default_group_);
    return (group == NULL)?NULL:group->vo;
  };
  const char* default_subject(void) const { return subject.c_str(); };
  // Returns all VOMS attributes associated with user
  const std::vector<struct voms_t>& voms(void) const;
  // Returns all internal (locally configured) VOs associated with user
  const std::list<std::string>& VOs(void) const;
  // convert ARC VOMS attribute list into voms structure
  static struct voms_t arc_to_voms(const std::string& vo,const std::vector<std::string>& attributes);

  static std::string err_to_string(int err);
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
  AuthResult evaluate(AuthUser &) const;
  bool operator==(const char* s) { return (strcmp(name.c_str(),s)==0); };
  bool operator==(const std::string& s) const { return (name == s); };
  const char* get_name() const { return name.c_str(); };
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

#endif 
