#ifndef __GM_ENVIRONMENT_H__
#define __GM_ENVIRONMENT_H__

#include <string>

//#include "../log/job_log.h"

class JobLog;
class JobsListConfig;

class GMEnvironment {
  bool valid_;
  JobLog& job_log_;
  JobsListConfig& jobs_cfg_;
 public:
  GMEnvironment(JobLog& job_log,JobsListConfig& jcfg,bool guess = false);
  operator bool(void) { return valid_; };
  bool operator!(void) { return !valid_; };

  /// ARC installation path - $ARC_LOCATION, executable path
  std::string nordugrid_loc(void) const;
  /// ARC system tools - $ARC_LOCATION/libexec/arc, $ARC_LOCATION/libexec
  std::string nordugrid_libexec_loc(void) const;
  // ARC libraries and plugins - $ARC_LOCATION/lib/arc, $ARC_LOCATION/lib
  std::string nordugrid_lib_loc(void) const;
  // ARC adminstrator tools - $ARC_LOCATION/sbin
  std::string nordugrid_sbin_loc(void) const;
  /// ARC configuration file 
  ///   /etc/arc.conf
  ///   $ARC_LOCATION/etc/arc.conf
  std::string nordugrid_config_loc(void) const;
  void nordugrid_config_loc(const std::string&);

  // Certificates directory location
  std::string cert_dir_loc() const;

  // RTE setup scripts
  std::string runtime_config_dir(void) const;
  void runtime_config_dir(const std::string&);

  /// Email address of person responsible for this ARC installation
  /// grid.manager@hostname, it can also be set from configuration file 
  std::string support_mail_address(void) const;
  void support_mail_address(const std::string&);

  JobLog& job_log() const;
  JobsListConfig& jobs_cfg() const;

};

///  Read environment, check files and set variables
///  Accepts:
///    guess - if false, default values are not allowed.
///  Returns:
///    true - success.
///    false - something is missing.
//bool read_env_vars(bool guess = false);

#endif // __GM_ENVIRONMENT_H__
