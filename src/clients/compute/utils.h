#ifndef __ARC_CLIENT_COMPUTE_UTILS_H_
#define __ARC_CLIENT_COMPUTE_UTILS_H_

#include <unistd.h>
#include <string>
#include <list>
#include <arc/Logger.h>
#include <arc/UserConfig.h>
#include <arc/OptionParser.h>
#include <arc/compute/Endpoint.h>
#include <arc/compute/Job.h>
#include <arc/compute/JobInformationStorage.h>
#include <arc/compute/JobDescription.h>

struct termios;

// This class records current state of console
// when created and recovers it when destroyed.
// Its main purpose is to recover console in 
// case application had to cancel any UI actions
// involving changing console state like 
// password input.
class ConsoleRecovery {
 private:
  ConsoleRecovery(ConsoleRecovery const&);
  ConsoleRecovery& operator=(ConsoleRecovery const&);
  struct termios * ti;
 public:
  ConsoleRecovery(void);
  ~ConsoleRecovery(void);
};

#ifdef TEST
#define RUNMAIN(X) test_##X##_main
#else
#define RUNMAIN(X) X(int argc, char **argv); \
  int main(int argc, char **argv) { int xr = 0; { ConsoleRecovery cr; xr = X(argc,argv); }; _exit(xr); return 0; } \
  int X
#endif


/// Returns the URLs of computing elements selected by alias, group name, URL or the default ones
/**
  This helper method gets a list of string representing computing elements. Each item of the list
  is either an alias of service configured in the UserConfig, a name of a group configured in the
  UserConfig, or a URL of service not configured in the UserConfig. If the list is empty, the
  default services will be selected from the UserConfig. The method returns the URLs of the
  selected services.
  
  This is meant to be used by the command line programs where the user is specifying
  a list of computing elements by alias, group name (which has to be looked up in the UserConfig),
  or by URL.
  \param[in] usercfg is the UserConfig object containing information about configured services
  \param[in] computingelements is a list of strings containing aliases, group names, or URLs of computing elements
  \return a list of URL strings, the endpoints of the selected services, or the default ones if none was selected
*/
std::list<std::string> getSelectedURLsFromUserConfigAndCommandLine(Arc::UserConfig usercfg, std::list<std::string> computingelements);

/// Combine the list of rejected discovery URLs from the UserConfig with the ones specified in a list
/**
  Helper method for the command line programs to combine the list of rejected discovery URLs
  specified by the user at the command line with the ones configured in the UserConfig.
  
  The rejected discovery URLs supposed to cause the service discovery not to discovery computing elements
  whose URL matches any of these strings.
  
  \param[in] usercfg is the UserConfig object containing information about configured services
  \param[in] rejectdiscovery is a list of strings, which will be also added
    to the resulting list besides the ones from the UserConfig
  \return a list of strings which are the rejected URLs from the UserConfig and
    the ones given as the second argument combined
*/
std::list<std::string> getRejectDiscoveryURLsFromUserConfigAndCommandLine(Arc::UserConfig usercfg, std::list<std::string> rejectdiscovery);

/// Combine the list of rejected management URLs from the UserConfig with the ones specified in a list
/**
  Helper method for the command line programs to combine the list of rejected management URLs
  specified by the user at the command line with the ones configured in the UserConfig.
  
  The rejected management URLs supposed to cause the job management commands not to manage
  jobs which reside on computing elements whose URL matches any of the items in the list
  
  \param[in] usercfg is the UserConfig object containing information about configured services
  \param[in] rejectmanagement is a list of strings, which will be also added
    to the resulting list besides the ones from the UserConfig
  \return a list of strings which are the rejected URLs from the UserConfig and
    the ones given as the second argument combined
*/
std::list<std::string> getRejectManagementURLsFromUserConfigAndCommandLine(Arc::UserConfig usercfg, std::list<std::string> rejectmanagement);

/// Looks up or creates Endpoints from strings specified at the command line using the information from the UserConfig
/**
  This helper method gets a list of strings representing service registries and computing element,
  along with a requested submisison interface, looks up all the services from the UserConfig,
  and return the Endpoints found there, or create new Endpoints for services not found in the Userconfig.
  If there are no registries or computing elements given, then the default services will be returned.
  
  This is meant to be used by the command line programs where the user is specifying service registries
  and/or computing elements with several strings, which could refer to services configured in the
  UserConfig (aliases or groups), or they can be URLs refering to services which are not configured in
  the UserConfig. This method looks up the aliases and group names, and if a string is not an alias or
  a group name, then it's assumed to be a URL.
  
  \param[in] usercfg is the UserConfig object containing information about configured services
  \param[in] registries is a list of strings containing aliases, group names, or URLs of service registries
  \param[in] computingelements is a list of strings containing aliases, group names, or URLs of computing elements
  \return a list of Endpoint objects containing the services corresponding the given strings or the default services.
*/
std::list<Arc::Endpoint> getServicesFromUserConfigAndCommandLine(Arc::UserConfig usercfg, std::list<std::string> registries, std::list<std::string> computingelements, std::string requestedSubmissionInterfaceName = "", std::string infointerface = "");

void showplugins(const std::string& program, const std::list<std::string>& types, Arc::Logger& logger, const std::string& chosenBroker = "");

bool checkproxy(const Arc::UserConfig& uc);

bool checktoken(const Arc::UserConfig& uc);

bool jobneedsproxy(const Arc::JobDescription& job);

void splitendpoints(std::list<std::string>& selected, std::list<std::string>& rejected);

/**
 * Creates a new JobInformationStorage object. Caller has responsibility of
 * deleting returned object.
 */
Arc::JobInformationStorage* createJobInformationStorage(const Arc::UserConfig& uc);

enum AuthenticationType {
  UndefinedAuthentication,
  NoAuthentication,
  X509Authentication,
  TokenAuthentication
};

enum DelegationType {
  UndefinedDelegation,
  NoDelegation,
  X509Delegation,
  TokenDelegation
};

class ClientOptions : public Arc::OptionParser {
public:
  enum Client_t {
    CO_SUB, CO_TEST,
    CO_CAT, CO_CLEAN, CO_GET, CO_KILL, CO_RENEW, CO_RESUME, CO_STAT,
    CO_SYNC,
    CO_INFO,
    CO_ACL
  };

  ClientOptions(Client_t c,
                const std::string& arguments = "",
                const std::string& summary = "",
                const std::string& description = "");

  /// Implement ARC consistent info/submission endpoint types logic
  bool canonicalizeARC6InterfaceTypes(Arc::Logger& logger);

  bool getDelegationType(Arc::Logger& logger, Arc::UserConfig const& usercfg, DelegationType& delegation_type) const;
  bool getAuthenticationType(Arc::Logger& logger, Arc::UserConfig const& usercfg, AuthenticationType& authentication_type) const;

  bool dryrun;
  bool dumpdescription;
  bool show_credentials;
  bool show_plugins;
  bool showversion;
  bool all;
  bool keep;
  bool forcesync;
  bool truncate;
  bool convert;
  bool longlist;
  bool printids;
  bool forceclean;
  bool show_stdout;
  bool show_stderr;
  bool show_joblog;
  bool show_json;
  bool usejobname;
  bool forcedownload;
  bool direct_submission;
  bool show_unavailable;
  bool no_delegation;
  bool x509_delegation;
  bool token_delegation;
  bool no_authentication;
  bool x509_authentication;
  bool token_authentication;
  bool force_system_ca;
  bool force_grid_ca;

  int testjobid;
  int runtime;
  int timeout;
  int instances_min;
  int instances_max;

  std::string show_file;

  std::string joblist;
  std::string jobidoutfile;
  std::string conffile;
  std::string debug;
  std::string broker;
  std::string sort;
  std::string rsort;
  std::string downloaddir;
  std::string requestedSubmissionInterfaceName;
  std::string infointerface;

  std::list<std::string> jobdescriptionstrings;
  std::list<std::string> jobdescriptionfiles;
  std::list<std::string> jobidinfiles;
  std::list<std::string> status;

  std::list<std::string> rejectdiscovery;
  std::list<std::string> rejectmanagement;

  // arc6 consistent, intuitive and streamlined target selection:
  // command line options
  std::list<std::string> computing_elements;
  std::list<std::string> registries;
  std::string requested_submission_endpoint_type;
  std::string requested_info_endpoint_type;
  // post-processed interface types
  std::list<std::string> submit_types;
  std::list<std::string> info_types;
};

#endif // __ARC_CLIENT_COMPUTE_UTILS_H_
