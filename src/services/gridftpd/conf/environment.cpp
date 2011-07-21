#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <arc/ArcLocation.h>
#include <arc/Utils.h>
#include <arc/Thread.h>
#include <arc/Logger.h>
#include "environment.h"


namespace gridftpd {

  static Arc::Logger logger(Arc::Logger::getRootLogger(),"GMEnvironment");

  static bool read_env_vars(bool guess);

  GMEnvironment::GMEnvironment(bool guess) {
    valid_=read_env_vars(guess);
  }

  class prstring {
   private:
    Glib::Mutex lock_;
    std::string val_;
   public:
    prstring(void);
    prstring(const char*);
    prstring(const prstring&);
    prstring& operator=(const char*);
    prstring& operator=(const std::string&);
    prstring& operator=(const prstring&);
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

  prstring::prstring(void) {
  }

  prstring::prstring(const char* val):lock_(), val_(val) {
  }

  prstring::prstring(const prstring& val):lock_(), val_(val.str()) {
  }

  prstring& prstring::operator=(const char* val) {
    Glib::Mutex::Lock lock(lock_);
    val_=val;
    return *this;
  }

  prstring& prstring::operator=(const std::string& val) {
    Glib::Mutex::Lock lock(lock_);
    val_=val;
    return *this;
  }

  prstring& prstring::operator=(const prstring& val) {
    if(&val == this) return *this;
    Glib::Mutex::Lock lock(lock_);
    val_=val.str();
    return *this;
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

  // ARC installation path - $ARC_LOCATION, executable path
  static prstring nordugrid_loc_;
  // ARC system tools
  static prstring nordugrid_libexec_loc_;
  // ARC libraries and plugins
  static prstring nordugrid_lib_loc_;
  // ARC administrator tools
  static prstring nordugrid_sbin_loc_;
  // ARC configuration file
  static prstring nordugrid_config_loc_;
  // Certificates directory
  static prstring cert_dir_loc_;
  // RTE setup scripts
  static prstring runtime_config_dir_;
  // Email address of person responsible for this ARC installation
  static prstring support_mail_address_;

  std::string GMEnvironment::nordugrid_loc(void) const {
    return nordugrid_loc_.str();
  }

  std::string GMEnvironment::nordugrid_libexec_loc(void) const {
    return nordugrid_libexec_loc_.str();
  }

  std::string GMEnvironment::nordugrid_lib_loc(void) const {
    return nordugrid_lib_loc_.str();
  }

  std::string GMEnvironment::nordugrid_sbin_loc(void) const {
    return nordugrid_sbin_loc_.str();
  }

  std::string GMEnvironment::nordugrid_config_loc(void) const {
    return nordugrid_config_loc_.str();
  }

  std::string GMEnvironment::cert_dir_loc(void) const {
    return cert_dir_loc_.str();
  }

  void GMEnvironment::nordugrid_config_loc(const std::string& val) {
    nordugrid_config_loc_=val;
  }

  std::string GMEnvironment::runtime_config_dir(void) const {
    return runtime_config_dir_.str();
  }

  void GMEnvironment::runtime_config_dir(const std::string& val) {
    runtime_config_dir_=val;
  }

  std::string GMEnvironment::support_mail_address(void) const {
    return support_mail_address_.str();
  }

  void GMEnvironment::support_mail_address(const std::string& val) {
    support_mail_address_=val;
  }


  static bool file_exists(const char* name) {
    struct stat st;
    if(stat(name,&st) != 0) return false;
    if(!S_ISREG(st.st_mode)) return false;
    return true;
  }


  static bool read_env_vars(bool guess) {
    if(nordugrid_loc_.empty()) {
      nordugrid_loc_=Arc::GetEnv("ARC_LOCATION");
      if(nordugrid_loc_.empty()) {
        nordugrid_loc_=Arc::ArcLocation::Get();
      };
      nordugrid_lib_loc_=nordugrid_loc_+"/"+PKGLIBSUBDIR;
      nordugrid_libexec_loc_=nordugrid_loc_+"/"+PKGLIBEXECSUBDIR;
    };

    if(nordugrid_config_loc_.empty()) {
      std::string tmp = Arc::GetEnv("ARC_CONFIG");
      if(tmp.empty()) {
        tmp=Arc::GetEnv("NORDUGRID_CONFIG");
        if(tmp.empty() && guess) {
          tmp="/etc/arc.conf";
          nordugrid_config_loc_=tmp;
          if(!file_exists(tmp.c_str())) {
            logger.msg(Arc::ERROR, "Central configuration file is missing at guessed location:\n"
                "  /etc/arc.conf\n"
                "Use ARC_CONFIG variable for non-standard location");
            return false;
          };
        };
      };
      if(!tmp.empty()) nordugrid_config_loc_=tmp;
    };

    if(cert_dir_loc_.empty()) {
      cert_dir_loc_=Arc::GetEnv("X509_CERT_DIR");
    };
  
    // Set all environement variables for other tools
    Arc::SetEnv("ARC_CONFIG",nordugrid_config_loc_);
    if(support_mail_address_.empty()) {
      char hn[100];
      support_mail_address_="grid.manager@";
      if(gethostname(hn,99) == 0) {
        support_mail_address_+=hn;
      }
      else {
        support_mail_address_+="localhost";
      };
    };
    return true;
  }

} // namespace gridftpd
