#ifndef __GM_ENVIRONMENT_H__
#define __GM_ENVIRONMENT_H__

#include <string>

/// Globus installation path - $GLOBUS_LOCATION, /opt/globus
extern std::string globus_loc;
// Various Globus scripts - $GLOBUS_LOCATION/libexec
extern std::string globus_scripts_loc;
/// ARC installation path - $ARC_LOCATION, $NORDUGRID_LOCATION, executable path
extern std::string nordugrid_loc;
/// ARC user tools - $ARC_LOCATION/bin
extern std::string nordugrid_bin_loc;
/// ARC system tools - $ARC_LOCATION/libexec/nordugrid, $ARC_LOCATION/libexec
extern std::string nordugrid_libexec_loc;
// ARC libraries and plugins - $ARC_LOCATION/lib/nordugrid, $ARC_LOCATION/lib
extern std::string nordugrid_lib_loc;
/// ARC configuration file 
///   /etc/arc.conf
///   /etc/nordugrid.conf
///   $ARC_LOCATION/etc/nordugrid_config_basename
///   /etc/nordugrid_config_basename
extern std::string nordugrid_config_loc;
/// Email address of person responsible for this ARC installation
/// grid.manager@hostname, it can also be set from configuration file 
extern std::string support_mail_address;
/// Global gridmap files with welcomed users' DNs and UNIX names
/// $GRIDMAP, default /etc/grid-security/grid-mapfile
extern std::string globus_gridmap;
/// If central configuration fle should be used (default is yes)
extern bool central_configuration;
/// Name of non-central configuration file. Modify before calling functions.
extern const char* nordugrid_config_basename;

///  Read environment, check files and set variables
///  Accepts:
///    guess - if false, default values are not allowed.
///  Returns:
///    true - success.
///    false - something is missing.
bool read_env_vars(bool guess = false);

#endif // __GM_ENVIRONMENT_H__
