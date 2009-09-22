#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <arc/ArcLocation.h>
#include <arc/Utils.h>
#define olog std::cerr
#include "environment.h"

prstring::prstring(void) {
}

prstring::prstring(const char* val):val_(val) {
}

prstring::prstring(const prstring& val):val_(val.str()) {
}

void prstring::operator=(const char* val) {
  Glib::Mutex::Lock lock(lock_);
  val_=val;
}

void prstring::operator=(const std::string& val) {
  Glib::Mutex::Lock lock(lock_);
  val_=val;
}

void prstring::operator=(const prstring& val) {
  if(&val == this) return;
  Glib::Mutex::Lock lock(lock_);
  val_=val.str();
}

void prstring::operator+=(const char* val) {
  Glib::Mutex::Lock lock(lock_);
  val_+=val;
}

void prstring::operator+=(const std::string& val) {
  Glib::Mutex::Lock lock(lock_);
  val_+=val;
}

std::string prstring::operator+(const char* val) const {
  const_cast<Glib::Mutex&>(lock_).lock();
  std::string r = val_ + val;
  const_cast<Glib::Mutex&>(lock_).unlock();
  return r;
}

std::string prstring::operator+(const std::string& val) const {
  const_cast<Glib::Mutex&>(lock_).lock();
  std::string r = val_ + val;
  const_cast<Glib::Mutex&>(lock_).unlock();
  return r;
}

prstring::operator std::string(void) const {
  const_cast<Glib::Mutex&>(lock_).lock();
  std::string r = val_;
  const_cast<Glib::Mutex&>(lock_).unlock();
  return r;
}

bool prstring::empty() const {
  const_cast<Glib::Mutex&>(lock_).lock();
  bool r = val_.empty();
  const_cast<Glib::Mutex&>(lock_).unlock();
  return r;
}

std::string prstring::str(void) const {
  return operator std::string();
}

std::string operator+(const char* val1,const prstring& val2) {
  return (val1 + val2.str());
}

std::string operator+(const std::string& val1,const prstring& val2) {
  return (val1 + val2.str());
}

// Globus installation path - $GLOBUS_LOCATION
prstring globus_loc(""); 
// Various Globus scripts - $GLOBUS_LOCATION/libexec
prstring globus_scripts_loc;
// ARC installation path - $ARC_LOCATION, executable path
prstring nordugrid_loc("");
// ARC system tools
prstring nordugrid_libexec_loc;
// ARC libraries and plugins
prstring nordugrid_lib_loc;
// ARC administrator tools
prstring nordugrid_sbin_loc;
// ARC configuration file
prstring nordugrid_config_loc("");
// RTE setup scripts
prstring runtime_config_dir("");
// Email address of person responsible for this ARC installation
prstring support_mail_address;
// Global gridmap files with welcomed users' DNs and UNIX names
prstring globus_gridmap;

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
  if(globus_loc.empty()) {
    globus_loc=Arc::GetEnv("GLOBUS_LOCATION");
    if(globus_loc.empty()) {
      if(!guess) {
        olog<<"Warning: GLOBUS_LOCATION environment variable not defined"<<std::endl;
        //return false;
      }
      else {
        globus_loc="/opt/globus";
      };
    };
    Arc::SetEnv("GLOBUS_LOCATION",globus_loc);
  };
  globus_scripts_loc=globus_loc+"/libexec";

  if(nordugrid_loc.empty()) {
    nordugrid_loc=Arc::GetEnv("ARC_LOCATION");
    if(nordugrid_loc.empty()) {
      nordugrid_loc=Arc::ArcLocation::Get();
    };
    nordugrid_lib_loc=nordugrid_loc+"/"+PKGLIBSUBDIR;
    nordugrid_libexec_loc=nordugrid_loc+"/"+PKGLIBEXECSUBDIR;
  };

  if(nordugrid_config_loc.empty()) {
    std::string tmp = Arc::GetEnv("ARC_CONFIG");
    if(tmp.empty()) {
      tmp=Arc::GetEnv("NORDUGRID_CONFIG");
      if(tmp.empty()) {
        tmp="/etc/arc.conf";
        nordugrid_config_loc=tmp;
        if(!file_exists(tmp.c_str())) {
          olog<<"Central configuration file is missing at guessed location:\n"
              <<"  /etc/arc.conf\n"
              <<"Use ARC_CONFIG variable for non-standard location"
              <<std::endl;
          return false;
        };
      };
    };
    if(!tmp.empty()) nordugrid_config_loc=tmp;
  };
  // Set all environement variables for other tools
  Arc::SetEnv("ARC_CONFIG",nordugrid_config_loc);
  if(support_mail_address.empty()) {
    char hn[100];
    support_mail_address="grid.manager@";
    if(gethostname(hn,99) == 0) {
      support_mail_address+=hn;
    }
    else {
      support_mail_address+="localhost";
    };
  };
  std::string tmp=Arc::GetEnv("GRIDMAP");
  if(tmp.empty()) { globus_gridmap="/etc/grid-security/grid-mapfile"; }
  else { globus_gridmap=tmp; };
  return true;
}

