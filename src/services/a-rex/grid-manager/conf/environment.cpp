#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <arc/ArcLocation.h>
#define olog std::cerr
#define DEFAULT_ARC_LOCATION "/usr"
#include "environment.h"

#ifdef HAVE_GLIBMM_GETENV
#include <glibmm/miscutils.h>
#define GetEnv(NAME) Glib::getenv(NAME)
#else
#define GetEnv(NAME) (getenv(NAME)?getenv(NAME):"")
#endif

#ifdef HAVE_GLIBMM_SETENV
#include <glibmm/miscutils.h>
#define SetEnv(NAME,VALUE) Glib::setenv(NAME,VALUE)
#else
#define SetEnv(NAME,VALUE) setenv(NAME,VALUE,1)
#endif

// Globus installation path - $GLOBUS_LOCATION
std::string globus_loc(""); 
// Various Globus scripts - $GLOBUS_LOCATION/libexec
std::string globus_scripts_loc;
// ARC installation path - $ARC_LOCATION, executable path
std::string nordugrid_loc("");
// ARC user tools - $ARC_LOCATION/bin
std::string nordugrid_bin_loc;
// ARC system tools - $ARC_LOCATION/libexec
std::string nordugrid_libexec_loc;
// ARC libraries and plugins - $ARC_LOCATION/lib
std::string nordugrid_lib_loc;
// ARC configuration file
std::string nordugrid_config_loc("");
// Email address of person responsible for this ARC installation
std::string support_mail_address;
// Global gridmap files with welcomed users' DNs and UNIX names
std::string globus_gridmap;
// Name of non-central configuration file. Here it is for grid-manager, 
// other modules has to modify this variable
const char* nordugrid_config_basename = "grid-manager.conf";
bool central_configuration = true;

static bool file_exists(const char* name) {
  struct stat st;
  if(lstat(name,&st) != 0) return false;
  if(!S_ISREG(st.st_mode)) return false;
  return true;
}

static bool dir_exists(const char* name) {
  struct stat st;
  if(lstat(name,&st) != 0) return false;
  if(!S_ISDIR(st.st_mode)) return false;
  return true;
}

bool read_env_vars(bool guess) {
  if(globus_loc.length() == 0) {
    globus_loc=GetEnv("GLOBUS_LOCATION");
    if(globus_loc.empty()) {
      if(!guess) {
        olog<<"Warning: GLOBUS_LOCATION environment variable not defined"<<std::endl;
        //return false;
      }
      else {
        globus_loc="/opt/globus";
      };
    };
    SetEnv("GLOBUS_LOCATION",globus_loc);
  };
  globus_scripts_loc=globus_loc+"/libexec";
  if(nordugrid_loc.empty()) {
    nordugrid_loc=GetEnv("ARC_LOCATION");
    if(nordugrid_loc.empty()) {
      nordugrid_loc=Arc::ArcLocation::Get();
      if(nordugrid_loc.empty()) {
        if(!guess) {
          olog<<"ARC_LOCATION environment variable is not defined"<<std::endl;
          return false;
        } else {
          nordugrid_loc=DEFAULT_ARC_LOCATION;
        };
      };
    };
    SetEnv("ARC_LOCATION",nordugrid_loc);
  };
  nordugrid_bin_loc=nordugrid_loc+"/bin";
  // Try /usr installation first
  nordugrid_libexec_loc=nordugrid_loc+"/libexec/nordugrid";
  nordugrid_lib_loc=nordugrid_loc+"/libexec/nordugrid";
  if(!dir_exists(nordugrid_libexec_loc.c_str())) {
    nordugrid_libexec_loc=nordugrid_loc+"/libexec";
    nordugrid_lib_loc=nordugrid_loc+"/lib";
  };
  if(nordugrid_config_loc.empty()) {
    std::string tmp = GetEnv("ARC_CONFIG");
    if(tmp.empty()) {
      tmp=GetEnv("NORDUGRID_CONFIG");
      if(tmp.empty()) {
        if(!central_configuration) {
          nordugrid_config_loc=nordugrid_loc+"/etc/"+nordugrid_config_basename;
          if(!file_exists(nordugrid_config_loc.c_str())) {
            nordugrid_config_loc=std::string("/etc/")+nordugrid_config_basename;
          };
          if(!file_exists(nordugrid_config_loc.c_str())) {
            olog<<"Configation file is missing at all guessed locations:\n"
                <<"  "<<nordugrid_loc<<"/etc/"<<nordugrid_config_basename<<"\n"
                <<"  /etc/"<<nordugrid_config_basename<<"\n"
                <<"Use ARC_CONFIG variable for non-standard location"
                <<std::endl;
            return false;
          };
        } else {
          nordugrid_config_loc="/etc/arc.conf";
          if(!file_exists(nordugrid_config_loc.c_str())) {
            nordugrid_config_loc="/etc/nordugrid.conf";
          };
          if(!file_exists(nordugrid_config_loc.c_str())) {
            olog<<"Central configuration file is missing at guessed locations:\n"
                <<"  /etc/nordugrid.conf\n"
                <<"  /etc/arc.conf\n"
                <<"Use ARC_CONFIG variable for non-standard location"
                <<std::endl;
            return false;
          };
        };
      };
    };
    if(!tmp.empty()) nordugrid_config_loc=tmp;
  };
  // Set all environement variables for other tools
  SetEnv("ARC_CONFIG",nordugrid_config_loc.c_str());
  SetEnv("ARC_LOCATION",nordugrid_loc.c_str());
  if(support_mail_address.length() == 0) {
    char hn[100];
    support_mail_address="grid.manager@";
    if(gethostname(hn,99) == 0) {
      support_mail_address+=hn;
    }
    else {
      support_mail_address+="localhost";
    };
  };
  std::string tmp=GetEnv("GRIDMAP");
  if(tmp.empty()) { globus_gridmap="/etc/grid-security/grid-mapfile"; }
  else { globus_gridmap=tmp; };
  return true;
}

