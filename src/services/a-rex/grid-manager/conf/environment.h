#ifndef __GM_ENVIRONMENT_H__
#define __GM_ENVIRONMENT_H__

#include <string>

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

/// Globus installation path - $GLOBUS_LOCATION, /opt/globus
extern prstring globus_loc;
// Various Globus scripts - $GLOBUS_LOCATION/libexec
extern prstring globus_scripts_loc;
/// ARC installation path - $ARC_LOCATION, executable path
extern prstring nordugrid_loc;
/// ARC system tools - $ARC_LOCATION/libexec/arc, $ARC_LOCATION/libexec
extern prstring nordugrid_libexec_loc;
// ARC libraries and plugins - $ARC_LOCATION/lib/arc, $ARC_LOCATION/lib
extern prstring nordugrid_lib_loc;
// ARC adminstrator tools - $ARC_LOCATION/sbin
extern prstring nordugrid_sbin_loc;
/// ARC configuration file 
///   /etc/arc.conf
///   $ARC_LOCATION/etc/arc.conf
extern prstring nordugrid_config_loc;
/// Email address of person responsible for this ARC installation
/// grid.manager@hostname, it can also be set from configuration file 
extern prstring support_mail_address;
/// Global gridmap files with welcomed users' DNs and UNIX names
/// $GRIDMAP, default /etc/grid-security/grid-mapfile
extern prstring globus_gridmap;

///  Read environment, check files and set variables
///  Accepts:
///    guess - if false, default values are not allowed.
///  Returns:
///    true - success.
///    false - something is missing.
bool read_env_vars(bool guess = false);

#endif // __GM_ENVIRONMENT_H__
