#ifndef __GM_ENVIRONMENT_H__
#define __GM_ENVIRONMENT_H__

#include <string>

/*
#include <arc/Thread.h>

class prstring {
 private:
  Glib::Mutex lock_;
  std::string val_;
 public:
  prstring(void);
  prstring(const char*);
  prstring(const prstring&);
  void operator=(const char*);
  void operator=(const std::string&);
  void operator=(const prstring&);
  void operator+=(const char*);
  void operator+=(const std::string&);
  std::string operator+(const char*) const;
  std::string operator+(const std::string&) const;
  operator std::string(void) const;
  std::string str(void) const;
  bool empty() const;
};

std::string operator+(const char*,const prstring&);
std::string operator+(const std::string&,const prstring&);
*/

/// Globus installation path - $GLOBUS_LOCATION, /opt/globus
std::string globus_loc(void);
// Various Globus scripts - $GLOBUS_LOCATION/libexec
std::string globus_scripts_loc(void);
/// ARC installation path - $ARC_LOCATION, executable path
std::string nordugrid_loc(void);
/// ARC system tools - $ARC_LOCATION/libexec/arc, $ARC_LOCATION/libexec
std::string nordugrid_libexec_loc(void);
// ARC libraries and plugins - $ARC_LOCATION/lib/arc, $ARC_LOCATION/lib
std::string nordugrid_lib_loc(void);
// ARC adminstrator tools - $ARC_LOCATION/sbin
std::string nordugrid_sbin_loc(void);
/// ARC configuration file 
///   /etc/arc.conf
///   $ARC_LOCATION/etc/arc.conf
std::string nordugrid_config_loc(void);
void nordugrid_config_loc(const std::string&);
// RTE setup scripts
std::string runtime_config_dir(void);
void runtime_config_dir(const std::string&);
/// Email address of person responsible for this ARC installation
/// grid.manager@hostname, it can also be set from configuration file 
std::string support_mail_address(void);
void support_mail_address(const std::string&);
/// Global gridmap files with welcomed users' DNs and UNIX names
/// $GRIDMAP, default /etc/grid-security/grid-mapfile
std::string globus_gridmap(void);
/// Whether Janitor is enabled. Empty value means disabled (the default)
std::string janitor_enabled(void);
void janitor_enabled(const std::string&);

///  Read environment, check files and set variables
///  Accepts:
///    guess - if false, default values are not allowed.
///  Returns:
///    true - success.
///    false - something is missing.
bool read_env_vars(bool guess = false);

#endif // __GM_ENVIRONMENT_H__
