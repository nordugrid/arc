#include <string>
#include <list>
#include <vector>

#include <string.h>

#include <arc/ArcConfig.h>
#include <arc/message/Message.h>
//#include <arc/message/SecHandler.h>

namespace ArcSHCLegacy {

#define AAA_POSITIVE_MATCH 1
#define AAA_NEGATIVE_MATCH -1
#define AAA_NO_MATCH 0
#define AAA_FAILURE 2

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
  typedef int (AuthUser:: * match_func_t)(const char* line);
  typedef struct {
    const char* cmd;
    match_func_t func;
  } source_t;
  class group_t {
   public:
    std::string name;             //
    const char* vo;               //
    struct voms_t voms;           //
    group_t(const std::string& name_,const char* vo_,const struct voms_t& voms_):name(name_),vo(vo_?vo_:""),voms(voms_) { };
  };

  const char* default_vo_;
  struct voms_t default_voms_;
  const char* default_group_;

  // Attributes of user
  std::string subject_;   // DN of certificate
  std::vector<struct voms_t> voms_data_; // VOMS information extracted from message

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
  int evaluate(const char* line);
  const char* DN(void) const { return subject_.c_str(); };
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
  void get_vos(std::list<std::string>& vos) const;
  const std::vector<struct voms_t>& voms(void);
  const std::list<std::string>& VOs(void);
  // convert ARC list into voms structure
  static std::vector<struct voms_t> arc_to_voms(const std::list<std::string>& attributes);
  /*
   * Get a certain property of the AuthUser, for example DN
   * or VOMS VO. For possible values of property see the source
   * code in auth.cc
   *
   * Not used in gridftpd
   */
  const std::string get_property(const std::string /* property */) const {
    return std::string("");
  };
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

