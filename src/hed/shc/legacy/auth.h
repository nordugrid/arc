#include <string>
#include <list>
#include <vector>

#include <string.h>

#include <arc/ArcConfig.h>
#include <arc/message/Message.h>
//#include <arc/message/SecHandler.h>

namespace ArcSHCLegacy {

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

struct otokens_t {
  std::string subject;
  std::string issuer;
  std::string audience;
  std::list<std::string> scopes;
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
    const char* vo;               // local VO which caused authorization of this group
    struct voms_t voms;           // VOMS attributes which caused authorization of this group
    struct otokens_t otokens;     // OTokens attributes which caused authorization of this group
    group_t(const std::string& name_,const char* vo_,const struct voms_t& voms_,const struct otokens_t& otokens_):
                                  name(name_),vo(vo_?vo_:""),voms(voms_),otokens(otokens_) { };
  };

  struct voms_t default_voms_;
  struct otokens_t default_otokens_;
  const char* default_vo_;
  const char* default_group_;

  // Attributes of user
  std::string subject_;   // DN of certificate
  std::vector<struct voms_t> voms_data_; // VOMS information extracted from message
  std::vector<struct otokens_t> otokens_data_; // OTokens information extracted from message

  // Old attributes - remove or convert
  std::string from;      // Remote hostname
  std::string filename;  // Delegated proxy stored in this file
  bool proxy_file_was_created; // If proxy file was created by this object
  bool has_delegation;   // If proxy contains delegation 

  // Matching methods
  static source_t sources[]; // Supported evaluation sources
  AuthResult match_all(const char* line);
  AuthResult match_group(const char* line);
  AuthResult match_subject(const char* line);
  AuthResult match_file(const char* line);
  AuthResult match_ldap(const char* line);
  AuthResult match_voms(const char* line);
  AuthResult match_otokens(const char* line);
  AuthResult match_vo(const char* line);
  AuthResult match_lcas(const char *);
  AuthResult match_plugin(const char* line);

  const group_t* find_group(const char* grp) const {
    if(grp == NULL) return NULL;
    for(std::list<group_t>::const_iterator i=groups_.begin();i!=groups_.end();++i) {
      if(i->name == grp) return &(*i);
    };
    return NULL;
  };
  const group_t* find_group(const std::string& grp) const { return find_group(grp.c_str());};

//  bool voms_extracted;

  // Evaluation results
  std::list<group_t> groups_; // Groups which user matched (internal names)
  std::list<std::string> vos_; // VOs to which user belongs (external names)

  // References to related/source data
  Arc::Message& message_;

 public:
  AuthUser(const AuthUser&);
  // Constructor
  AuthUser(Arc::Message& message);

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
  AuthResult evaluate(const char* line);
  const char* subject(void) const { return subject_.c_str(); };
  const char* proxy(void) const {
    (const_cast<AuthUser*>(this))->store_credentials();
    return filename.c_str();
  };
  bool is_proxy(void) const { return has_delegation; };
  const char* hostname(void) const { return from.c_str(); };
  // Remember this user belongs to group 'grp'
  void add_group(const std::string& grp);
  void add_groups(const std::list<std::string>& grps);
  // Mark this user as belonging to no no groups
  void clear_groups(void) { groups_.clear(); default_group_=NULL; };
  // Returns true if user belongs to specified group 'grp'
  bool check_group(const std::string& grp) const {
    for(std::list<group_t>::const_iterator i=groups_.begin();i!=groups_.end();++i) {
      if(i->name == grp) return true;
    };
    return false;
  };
  void get_groups(std::list<std::string>& groups) const;
  void add_vo(const std::string& vo);
  void add_vos(const std::list<std::string>& vos);
  void clear_vos(void) { vos_.clear(); };
  bool check_vo(const std::string& vo) const {
    for(std::list<std::string>::const_iterator i=vos_.begin();i!=vos_.end();++i) {
      if(*i == vo) return true;
    };
    return false;
  };
  const std::vector<struct voms_t>& voms(void);
  const std::vector<struct otokens_t>& otokens(void);
  const std::list<std::string>& VOs(void);
  const struct voms_t* get_group_voms(const std::string& grp) const {
    const group_t* group = find_group(grp);
    return (group == NULL)?NULL:&(group->voms);
  };
  const char* get_group_vo(const std::string& grp) const {
    const group_t* group = find_group(grp);
    return (group == NULL)?NULL:group->vo;
  };
  const struct otokens_t* get_group_otokens(const std::string& grp) const {
    const group_t* group = find_group(grp);
    return (group == NULL)?NULL:&(group->otokens);
  };


  // convert ARC list into voms structure
  static std::vector<struct voms_t> arc_to_voms(const std::list<std::string>& attributes);
  void subst(std::string& str);
  bool store_credentials(void);
};

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


} // namespace ArcSHCLegacy

