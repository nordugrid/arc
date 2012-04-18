// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <fstream>
#include <glibmm.h>
#include <arc/FileUtils.h>

#include <arc/ArcLocation.h>
#include <arc/IniConfig.h>
#include <arc/Logger.h>
#include <arc/StringConv.h>
#include <arc/URL.h>
#include <arc/User.h>
#include <arc/Utils.h>
#include "UserConfig.h"

#define HANDLESTRATT(ATT, SETTER) \
  if (common[ATT]) {\
    SETTER((std::string)common[ATT]);\
    common[ATT].Destroy();\
    if (common[ATT]) {\
      logger.msg(WARNING, "Multiple %s attributes in configuration file (%s)", ATT, conffile); \
      while (common[ATT]) common[ATT].Destroy();\
    }\
  }


namespace Arc {

  typedef enum {
    file_test_missing,
    file_test_not_file,
    file_test_wrong_ownership,
    file_test_wrong_permissions,
    file_test_success
  } file_test_status;

#ifndef WIN32
  static file_test_status user_file_test(const std::string& path, const User& user) {
    struct stat st;
    if(::stat(path.c_str(),&st) != 0) return file_test_missing;
    if(!S_ISREG(st.st_mode)) return file_test_not_file;
    if(user.get_uid() != st.st_uid) return file_test_wrong_ownership;
    return file_test_success;
  }

  static file_test_status private_file_test(const std::string& path, const User& user) {
    struct stat st;
    if(::stat(path.c_str(),&st) != 0) return file_test_missing;
    if(!S_ISREG(st.st_mode)) return file_test_not_file;
    if(user.get_uid() != st.st_uid) return file_test_wrong_ownership;
    if(st.st_mode & (S_IRWXG | S_IRWXO)) return file_test_wrong_permissions;
    return file_test_success;
  }
#else
  static file_test_status user_file_test(const std::string& path, const User& /*user*/) {
    // TODO: implement
    if(!Glib::file_test(path, Glib::FILE_TEST_EXISTS)) return file_test_missing;
    if(!Glib::file_test(path, Glib::FILE_TEST_IS_REGULAR)) return file_test_not_file;
    return file_test_success;
  }

  static file_test_status private_file_test(const std::string& path, const User& /*user*/) {
    // TODO: implement
    if(!Glib::file_test(path, Glib::FILE_TEST_EXISTS)) return file_test_missing;
    if(!Glib::file_test(path, Glib::FILE_TEST_IS_REGULAR)) return file_test_not_file;
    return file_test_success;
  }
#endif

  static void certificate_file_error_report(file_test_status fts, bool require, const std::string& path, Logger& logger) {
    if(fts == file_test_success) {
    } else if(fts == file_test_wrong_ownership) {
      logger.msg(require?ERROR:VERBOSE, "Wrong ownership of certificate file: %s", path);
    } else if(fts == file_test_wrong_permissions) {
      logger.msg(require?ERROR:VERBOSE, "Wrong permissions of certificate file: %s", path);
    } else {
      logger.msg(require?ERROR:VERBOSE, "Can not access certificate file: %s", path);
    }
  }

  static void key_file_error_report(file_test_status fts, bool require, const std::string& path, Logger& logger) {
    if(fts == file_test_success) {
    } else if(fts == file_test_wrong_ownership) {
      logger.msg(require?ERROR:VERBOSE, "Wrong ownership of key file: %s", path);
    } else if(fts == file_test_wrong_permissions) {
      logger.msg(require?ERROR:VERBOSE, "Wrong permissions of key file: %s", path);
    } else {
      logger.msg(require?ERROR:VERBOSE, "Can not access key file: %s", path);
    }
  }

  static void proxy_file_error_report(file_test_status fts, bool require, const std::string& path, Logger& logger) {
    if(fts == file_test_success) {
    } else if(fts == file_test_wrong_ownership) {
      logger.msg(require?ERROR:VERBOSE, "Wrong ownership of proxy file: %s", path);
    } else if(fts == file_test_wrong_permissions) {
      logger.msg(require?ERROR:VERBOSE, "Wrong permissions of proxy file: %s", path);
    } else {
      logger.msg(require?ERROR:VERBOSE, "Can not access proxy file: %s", path);
    }
  }

  static bool dir_test(const std::string& path) {
    return Glib::file_test(path, Glib::FILE_TEST_IS_DIR);
  }

  std::string tostring(const ServiceType st) {
    switch (st) {
    case COMPUTING:
      return istring("computing");
    case INDEX:
      return istring("index");
    }

    return "";
  }

  Logger UserConfig::logger(Logger::getRootLogger(), "UserConfig");

  const std::string UserConfig::DEFAULT_BROKER = "Random";

  const std::string UserConfig::ARCUSERDIRECTORY = Glib::build_filename(User().Home(), ".arc");

#ifndef WIN32
  const std::string UserConfig::SYSCONFIG = G_DIR_SEPARATOR_S "etc" G_DIR_SEPARATOR_S "arc" G_DIR_SEPARATOR_S "client.conf";
#else
  const std::string UserConfig::SYSCONFIG = ArcLocation::Get() + G_DIR_SEPARATOR_S "etc" G_DIR_SEPARATOR_S "arc" G_DIR_SEPARATOR_S "client.conf";
#endif
  const std::string UserConfig::SYSCONFIGARCLOC = ArcLocation::Get() + G_DIR_SEPARATOR_S "etc" G_DIR_SEPARATOR_S "arc" G_DIR_SEPARATOR_S "client.conf";
  const std::string UserConfig::EXAMPLECONFIG = ArcLocation::Get() + G_DIR_SEPARATOR_S PKGDATASUBDIR G_DIR_SEPARATOR_S "examples" G_DIR_SEPARATOR_S "client.conf";

  const std::string UserConfig::DEFAULTCONFIG = Glib::build_filename(ARCUSERDIRECTORY, "client.conf");

  UserConfig::UserConfig(initializeCredentialsType initializeCredentials)
    : ok(false), initializeCredentials(initializeCredentials) {
    if (!InitializeCredentials(initializeCredentials)) {
      return;
    }

    ok = true;

    setDefaults();
  }

  UserConfig::UserConfig(const std::string& conffile,
                         initializeCredentialsType initializeCredentials,
                         bool loadSysConfig)
    : ok(false), initializeCredentials(initializeCredentials)  {
    if (loadSysConfig) {
#ifndef WIN32
      if (Glib::file_test(SYSCONFIG, Glib::FILE_TEST_IS_REGULAR)) {
        if (!LoadConfigurationFile(SYSCONFIG, true))
          logger.msg(INFO, "System configuration file (%s) contains errors.", SYSCONFIG);
      }
      else
#endif
      if (Glib::file_test(SYSCONFIGARCLOC, Glib::FILE_TEST_IS_REGULAR)) {
        if (!LoadConfigurationFile(SYSCONFIGARCLOC, true))
          logger.msg(INFO, "System configuration file (%s) contains errors.", SYSCONFIGARCLOC);
      }
      else
#ifndef WIN32
        if (!ArcLocation::Get().empty() && ArcLocation::Get() != G_DIR_SEPARATOR_S)
          logger.msg(VERBOSE, "System configuration file (%s or %s) does not exist.", SYSCONFIG, SYSCONFIGARCLOC);
        else
          logger.msg(VERBOSE, "System configuration file (%s) does not exist.", SYSCONFIG);
#else
        logger.msg(VERBOSE, "System configuration file (%s) does not exist.", SYSCONFIGARCLOC);
#endif
    }

    if (conffile.empty()) {
      if (CreateDefaultConfigurationFile()) {
        if (!LoadConfigurationFile(DEFAULTCONFIG, false)) {
          logger.msg(WARNING, "User configuration file (%s) contains errors.", DEFAULTCONFIG);
          return;
        }
      }
      else
        logger.msg(INFO, "No configuration file could be loaded.");
    }
    else if (!Glib::file_test(conffile, Glib::FILE_TEST_IS_REGULAR)) {
      logger.msg(WARNING, "User configuration file (%s) does not exist or cannot be loaded.", conffile);
      return;
    }
    else if (!LoadConfigurationFile(conffile)) {
      logger.msg(WARNING, "User configuration file (%s) contains errors.", conffile);
      return;
    }

    if (!InitializeCredentials(initializeCredentials)) {
      return;
    }

    ok = true;

    setDefaults();
  }

  UserConfig::UserConfig(const std::string& conffile, const std::string& jfile,
                         initializeCredentialsType initializeCredentials, bool loadSysConfig)
    : ok(false), initializeCredentials(initializeCredentials)  {
    // If job list file have been specified, try to initialize it, and
    // if it fails then this object is non-valid (ok = false).
    if (!jfile.empty() && !JobListFile(jfile))
      return;

    if (loadSysConfig) {
#ifndef WIN32
      if (Glib::file_test(SYSCONFIG, Glib::FILE_TEST_IS_REGULAR)) {
        if (!LoadConfigurationFile(SYSCONFIG, true))
          logger.msg(INFO, "System configuration file (%s) contains errors.", SYSCONFIG);
      }
      else
#endif
      if (Glib::file_test(SYSCONFIGARCLOC, Glib::FILE_TEST_IS_REGULAR)) {
        if (!LoadConfigurationFile(SYSCONFIGARCLOC, true))
          logger.msg(INFO, "System configuration file (%s) contains errors.", SYSCONFIGARCLOC);
      }
      else
#ifndef WIN32
        if (!ArcLocation::Get().empty() && ArcLocation::Get() != G_DIR_SEPARATOR_S)
          logger.msg(VERBOSE, "System configuration file (%s or %s) does not exist.", SYSCONFIG, SYSCONFIGARCLOC);
        else
          logger.msg(VERBOSE, "System configuration file (%s) does not exist.", SYSCONFIG);
#else
        logger.msg(VERBOSE, "System configuration file (%s) does not exist.", SYSCONFIGARCLOC);
#endif
    }

    if (conffile.empty()) {
      if (CreateDefaultConfigurationFile()) {
        if (!LoadConfigurationFile(DEFAULTCONFIG, !jfile.empty())) {
          logger.msg(WARNING, "User configuration file (%s) contains errors.", DEFAULTCONFIG);
          return;
        }
      }
      else
        logger.msg(INFO, "No configuration file could be loaded.");
    }
    else if (!Glib::file_test(conffile, Glib::FILE_TEST_IS_REGULAR)) {
      logger.msg(WARNING, "User configuration file (%s) does not exist or cannot be loaded.", conffile);
      return;
    }
    else if (!LoadConfigurationFile(conffile, !jfile.empty())) {
      logger.msg(WARNING, "User configuration file (%s) contains errors.", conffile);
      return;
    }

    // If no job list file have been initialized use the default. If the
    // job list file cannot be initialized this object is non-valid.
    if (joblistfile.empty() && !JobListFile(Glib::build_filename(ARCUSERDIRECTORY, "jobs.xml")))
      return;

    if (!InitializeCredentials(initializeCredentials)) {
      return;
    }

    ok = true;

    setDefaults();
  }

  UserConfig::UserConfig(const long int& ptraddr) { *this = *((UserConfig*)ptraddr); }

  void UserConfig::ApplyToConfig(BaseConfig& ccfg) const {
    if (!proxyPath.empty())
      ccfg.AddProxy(proxyPath);
    else {
      ccfg.AddCertificate(certificatePath);
      ccfg.AddPrivateKey(keyPath);
    }

    ccfg.AddCADir(caCertificatesDirectory);

    if(!overlayfile.empty())
      ccfg.GetOverlay(overlayfile);
  }

  bool UserConfig::Timeout(int newTimeout) {
    if (timeout > 0) {
      timeout = newTimeout;
      return true;
    }

    return false;
  }

  bool UserConfig::Verbosity(const std::string& newVerbosity) {
    LogLevel ll;
    if (istring_to_level(newVerbosity, ll)) {
      verbosity = newVerbosity;
      return true;
    }
    else {
      logger.msg(WARNING, "Unable to parse the specified verbosity (%s) to one of the allowed levels", newVerbosity);
      return false;
    }
  }

  bool UserConfig::JobListFile(const std::string& path) {
    // Check if joblistfile exist.
    if (!Glib::file_test(path, Glib::FILE_TEST_EXISTS)) {
      // The joblistfile does not exist. Create a empty version, and if
      // this fails report an error and exit.
      const std::string joblistdir = Glib::path_get_dirname(path);

      // Check if the parent directory exist.
      if (!Glib::file_test(joblistdir, Glib::FILE_TEST_EXISTS)) {
        // Create directory.
        if (!makeDir(joblistdir)) {
          logger.msg(ERROR, "Unable to create %s directory", joblistdir);
          return false;
        }
      }
      else if (!dir_test(joblistdir)) {
        logger.msg(ERROR, "%s is not a directory, it is needed for the client to function correctly", joblistdir);
        return false;
      }

      NS ns;
      Config(ns).SaveToFile(path);
      logger.msg(INFO, "Created empty ARC job list file: %s", path);
    }
    else if (!Glib::file_test(path, Glib::FILE_TEST_IS_REGULAR)) {
      logger.msg(ERROR, "ARC job list file is not a regular file: %s", path);
      return false;
    }

    joblistfile = path;
    return true;
  }

  bool UserConfig::Broker(const std::string& nameandarguments) {
    const std::size_t pos = nameandarguments.find(":");
    if (pos != std::string::npos) // Arguments given in 'nameandarguments'
      broker = std::make_pair<std::string, std::string>(nameandarguments.substr(0, pos),
                                                        nameandarguments.substr(pos+1));
    else
      broker = std::make_pair<std::string, std::string>(nameandarguments, "");

    return true;
  }

  bool UserConfig::InitializeCredentials(initializeCredentialsType initializeCredentials) {
    if(initializeCredentials == initializeCredentialsType::SkipCredentials) return true;
    bool res = true;
    bool require =
            ((initializeCredentials == initializeCredentialsType::RequireCredentials) ||
             (initializeCredentials == initializeCredentialsType::SkipCARequireCredentials));
    bool test =
            ((initializeCredentials == initializeCredentialsType::RequireCredentials) ||
             (initializeCredentials == initializeCredentialsType::TryCredentials) ||
             (initializeCredentials == initializeCredentialsType::SkipCARequireCredentials) ||
             (initializeCredentials == initializeCredentialsType::SkipCATryCredentials)); 
    bool noca =
            ((initializeCredentials == initializeCredentialsType::SkipCARequireCredentials) ||
             (initializeCredentials == initializeCredentialsType::SkipCATryCredentials) ||
             (initializeCredentials == initializeCredentialsType::SkipCANotTryCredentials));
    const User user;
#ifndef WIN32
    std::string home_path = user.Home();
#else
    std::string home_path = Glib::get_home_dir();
#endif
    bool has_proxy = false;
    // Look for credentials.
    std::string proxy_path = GetEnv("X509_USER_PROXY");
    if (!proxy_path.empty()) {
      proxyPath = proxy_path;     
      file_test_status fts;
      if (test && ((fts = private_file_test(proxyPath, user)) != file_test_success)) {
        proxy_file_error_report(fts,require,proxyPath,logger);
        if(require) {
          //res = false;
        }
        proxyPath.clear();
      } else {
        has_proxy = true;
      }
    } else if (!proxyPath.empty()) {
      file_test_status fts;
      if (test && ((fts = private_file_test(proxyPath, user)) != file_test_success)) {
        proxy_file_error_report(fts,require,proxyPath,logger);
        if(require) {
          //res = false;
        }
        proxyPath.clear();
      } else {
        has_proxy = true;
      }
    } else {
      proxy_path = Glib::build_filename(Glib::get_tmp_dir(),
                             std::string("x509up_u") + tostring(user.get_uid()));
      proxyPath = proxy_path;
      file_test_status fts;
      if (test && ((fts = private_file_test(proxyPath, user)) != file_test_success)) {
        proxy_file_error_report(fts,require,proxyPath,logger);
        if (require) {
          // TODO: Maybe this message should be printed only after checking for key/cert
          // This is not error yet because there may be key/credentials
          //res = false;
        }
        proxyPath.clear();
      } else {
        has_proxy = true;
      }
    }

    if(has_proxy) require = false;
    // Should we really handle them in pairs
    std::string cert_path = GetEnv("X509_USER_CERT");
    std::string key_path = GetEnv("X509_USER_KEY");
    if (!cert_path.empty() && !key_path.empty()) {
      certificatePath = cert_path;
      file_test_status fts;
      if (test && ((fts = user_file_test(certificatePath, user)) != file_test_success)) {
        certificate_file_error_report(fts,require,certificatePath,logger);
        if(require) {
          res = false;
        }
        certificatePath.clear();
      }
      keyPath = key_path;
      if (test && ((fts = private_file_test(keyPath, user)) != file_test_success)) {
        key_file_error_report(fts,require,keyPath,logger);
        if(require) {
          res = false;
        }
        keyPath.clear();
      }
    } else if (!certificatePath.empty() && !keyPath.empty()) {
      file_test_status fts;
      if (test && ((fts = user_file_test(certificatePath, user)) != file_test_success)) {
        certificate_file_error_report(fts,require,certificatePath,logger);
        if(require) {
          res = false;
        }
        certificatePath.clear();
      }
      if (test && ((fts = private_file_test(keyPath, user)) != file_test_success)) {
        key_file_error_report(fts,require,keyPath,logger);
        if(require) {
          res = false;
        }
        keyPath.clear();
      }
    } else if (!certificatePath.empty() && (certificatePath.find(".p12") != std::string::npos) && keyPath.empty()) {
      //If only certificatePath provided, then it could be a pkcs12 file
      file_test_status fts;
      if (test && ((fts = user_file_test(certificatePath, user)) != file_test_success)) {
        certificate_file_error_report(fts,require,certificatePath,logger);
        if(require) {
          res = false;
        }
        certificatePath.clear();
      }
    } else if (!cert_path.empty() && (cert_path.find(".p12") != std::string::npos) && key_path.empty()) {
      //If only cert_path provided, then it could be a pkcs12 file
      file_test_status fts;
      if (test && ((fts = user_file_test(cert_path, user)) != file_test_success)) {
        certificate_file_error_report(fts,require,cert_path,logger);
        if(require) {
          res = false;
        }
        cert_path.clear();
      }
    } else {
      // Guessing starts here
      // First option is also main default
      std::string base_path = home_path+G_DIR_SEPARATOR_S+".arc"+G_DIR_SEPARATOR_S;
      cert_path = base_path+"usercert.pem";
      key_path = base_path+"userkey.pem";
      file_test_status fts1 = file_test_success;
      file_test_status fts2 = file_test_success;
      certificatePath = cert_path;
      keyPath = key_path;
      if( test && !(
          ((fts1 = user_file_test(certificatePath, user)) == file_test_success) &&
          ((fts2 = private_file_test(keyPath, user)) == file_test_success) ) ) {
        certificate_file_error_report(fts1,false,certificatePath,logger);
        key_file_error_report(fts2,false,keyPath,logger);
        fts1 = file_test_success; fts2 = file_test_success;
        base_path = home_path+G_DIR_SEPARATOR_S+".globus"+G_DIR_SEPARATOR_S;
        certificatePath = base_path+"usercert.pem";
        keyPath = base_path+"userkey.pem";
        if( test && !(
            ((fts1 = user_file_test(certificatePath, user)) == file_test_success) &&
            ((fts2 = private_file_test(keyPath, user)) == file_test_success) ) ) {
          certificate_file_error_report(fts1,false,certificatePath,logger);
          key_file_error_report(fts2,false,keyPath,logger);
          fts1 = file_test_success; fts2 = file_test_success;
          base_path = ArcLocation::Get()+G_DIR_SEPARATOR_S+"etc"+G_DIR_SEPARATOR_S+"arc"+G_DIR_SEPARATOR_S;
          certificatePath = base_path+"usercert.pem";
          keyPath = base_path+"userkey.pem";
          if( test && !(
              ((fts1 = user_file_test(certificatePath, user)) == file_test_success) &&
              ((fts2 = private_file_test(keyPath, user)) == file_test_success) ) ) {
            certificate_file_error_report(fts1,false,certificatePath,logger);
            key_file_error_report(fts2,false,keyPath,logger);
            fts1 = file_test_success; fts2 = file_test_success;
            // TODO: is it really safe to take credentials from ./ ? NOOOO
            base_path = Glib::get_current_dir() + G_DIR_SEPARATOR_S;
            certificatePath = base_path+"usercert.pem";
            keyPath = base_path+"userkey.pem";
            if( test && !(
                ((fts1 = user_file_test(certificatePath, user)) == file_test_success) &&
                ((fts2 = private_file_test(keyPath, user)) == file_test_success) ) ) {
              certificate_file_error_report(fts1,false,certificatePath,logger);
              key_file_error_report(fts2,false,keyPath,logger);
              fts1 = file_test_success; fts2 = file_test_success;
              // Not found
              logger.msg(require?WARNING:VERBOSE, 
                "Proxy certificate path was not explicitely set or does not exist or has\n"
                "improper permissions/ownership and not found at default location.\n"
                "Key/certificate paths were not explicitely set or do not exist or have\n"
                "improper permissions/ownership and usercert.pem/userkey.pem not found\n"
                "at default locations:\n"
                "~/.arc/, ~/.globus/, %s/etc/arc, and ./.\n"
                "Please manually specify the proxy or certificate/key locations, or use\n"
                "arcproxy utility to create a proxy certificte", ArcLocation::Get()
              );
              if(require) {
                res = false;
              }
              certificatePath.clear(); keyPath.clear();
            }
          }
        }
      }
    }

    if(!noca) {
      std::string ca_dir = GetEnv("X509_CERT_DIR");
      if (!ca_dir.empty()) {
        caCertificatesDirectory = ca_dir;
        if (test && !dir_test(caCertificatesDirectory)) {
          if(require) {
            logger.msg(WARNING, "Can not access CA certificates directory: %s. The certificates will not be verified.", caCertificatesDirectory);
            res = false;
          }
          caCertificatesDirectory.clear();
        }
      } else if (!caCertificatesDirectory.empty()) {
        if (test && !dir_test(caCertificatesDirectory)) {
          if(require) {
            logger.msg(WARNING, "Can not access CA certificate directory: %s. The certificates will not be verified.", caCertificatesDirectory);
            res = false;
          }
          caCertificatesDirectory.clear();
        }
      } else {
        caCertificatesDirectory = home_path+G_DIR_SEPARATOR_S+".arc"+G_DIR_SEPARATOR_S+"certificates";
        if (test && !dir_test(caCertificatesDirectory)) {
          caCertificatesDirectory = home_path+G_DIR_SEPARATOR_S+".globus"+G_DIR_SEPARATOR_S+"certificates";
          if (!dir_test(caCertificatesDirectory)) {
            caCertificatesDirectory = ArcLocation::Get()+G_DIR_SEPARATOR_S+"etc"+G_DIR_SEPARATOR_S+"certificates";
            if (!dir_test(caCertificatesDirectory)) {
              caCertificatesDirectory = ArcLocation::Get()+G_DIR_SEPARATOR_S+"etc"+G_DIR_SEPARATOR_S+"grid-security"+G_DIR_SEPARATOR_S+"certificates";
              if (!dir_test(caCertificatesDirectory)) {
                caCertificatesDirectory = ArcLocation::Get()+G_DIR_SEPARATOR_S+"share"+G_DIR_SEPARATOR_S+"certificates";
                if (!dir_test(caCertificatesDirectory)) {
                  caCertificatesDirectory = std::string(G_DIR_SEPARATOR_S)+"etc"+G_DIR_SEPARATOR_S+"grid-security"+G_DIR_SEPARATOR_S+"certificates";
                  if (!dir_test(caCertificatesDirectory)) {
                    if(require) {
                      logger.msg(WARNING,
                        "Can not find CA certificates directory in default locations:\n"
                        "~/.arc/certificates, ~/.globus/certificates,\n"
                        "%s/etc/certificates, %s/etc/grid-security/certificates,\n"
                        "%s/share/certificates, /etc/grid-security/certificates.\n"
                        "The certificate will not be verified.", 
                        ArcLocation::Get(), ArcLocation::Get(), ArcLocation::Get());
                      res = false;
                    }
                    caCertificatesDirectory.clear();
                  }
                }
              }
            }
          }
        }
      }
    }

    if (!proxyPath.empty()) {
      logger.msg(INFO, "Using proxy file: %s", proxyPath);
    }
    if ((!certificatePath.empty()) && (!keyPath.empty())) {
      logger.msg(INFO, "Using certificate file: %s", certificatePath);
      logger.msg(INFO, "Using key file: %s", keyPath);
    }

    if (!caCertificatesDirectory.empty()) {
      logger.msg(INFO, "Using CA certificate directory: %s", caCertificatesDirectory);
    }

    return res;
  }

  const std::string& UserConfig::VOMSESPath() {
    if(!vomsesPath.empty()) return vomsesPath;

    //vomsesPath could be regular file or directory, therefore only existence is checked here.
    //multiple voms paths under one vomsesPath is processed under arcproxy implementation.
    if (!GetEnv("X509_VOMS_FILE").empty()) {
      if (!Glib::file_test(vomsesPath = GetEnv("X509_VOMS_FILE"), Glib::FILE_TEST_EXISTS)) {
        logger.msg(WARNING, "Can not access VOMSES file/directory: %s.", vomsesPath);
        vomsesPath.clear();
      }
    }
    else if (!GetEnv("X509_VOMSES").empty()) {
      if (!Glib::file_test(vomsesPath = GetEnv("X509_VOMSES"), Glib::FILE_TEST_EXISTS)) {
        logger.msg(WARNING, "Can not access VOMSES file/directory: %s.", vomsesPath);
        vomsesPath.clear();
      }
    }
    else if (!vomsesPath.empty()) {
      if (!Glib::file_test(vomsesPath, Glib::FILE_TEST_EXISTS)) {
        logger.msg(WARNING, "Can not access VOMS file/directory: %s.", vomsesPath);
        vomsesPath.clear();
      }
    }
    else if (
#ifndef WIN32
             !Glib::file_test(vomsesPath = user.Home() + G_DIR_SEPARATOR_S + ".arc" + G_DIR_SEPARATOR_S + "vomses", Glib::FILE_TEST_EXISTS) &&
             !Glib::file_test(vomsesPath = user.Home() + G_DIR_SEPARATOR_S + ".voms" + G_DIR_SEPARATOR_S + "vomses", Glib::FILE_TEST_EXISTS) &&
#else
             !Glib::file_test(vomsesPath = std::string(Glib::get_home_dir()) + G_DIR_SEPARATOR_S + ".arc" + G_DIR_SEPARATOR_S + "vomses", Glib::FILE_TEST_EXISTS) &&
             !Glib::file_test(vomsesPath = std::string(Glib::get_home_dir()) + G_DIR_SEPARATOR_S + ".voms" + G_DIR_SEPARATOR_S + "vomses", Glib::FILE_TEST_EXISTS) &&
#endif
             !Glib::file_test(vomsesPath = ArcLocation::Get() + G_DIR_SEPARATOR_S + "etc" + G_DIR_SEPARATOR_S + "vomses", Glib::FILE_TEST_EXISTS) &&
             !Glib::file_test(vomsesPath = ArcLocation::Get() + G_DIR_SEPARATOR_S + "etc" + G_DIR_SEPARATOR_S + "grid-security" + G_DIR_SEPARATOR_S + "vomses", Glib::FILE_TEST_EXISTS) &&
             !Glib::file_test(vomsesPath = std::string(Glib::get_current_dir()) + G_DIR_SEPARATOR_S + "vomses", Glib::FILE_TEST_EXISTS)) {
      vomsesPath = Glib::build_filename(G_DIR_SEPARATOR_S + std::string("etc"), std::string("vomses"));
      if (!Glib::file_test(vomsesPath.c_str(), Glib::FILE_TEST_EXISTS)) {
        vomsesPath = Glib::build_filename(G_DIR_SEPARATOR_S + std::string("etc") + G_DIR_SEPARATOR_S + "grid-security", std::string("vomses"));
        if (!Glib::file_test(vomsesPath.c_str(), Glib::FILE_TEST_EXISTS)) {
          logger.msg(WARNING, "Can not find voms service configuration file (vomses) in default locations: ~/.arc/vomses, ~/.voms/vomses, $ARC_LOCATION/etc/vomses, $ARC_LOCATION/etc/grid-security/vomses, $PWD/vomses, /etc/vomses, /etc/grid-security/vomses");
          vomsesPath.clear();
        }
      }
    }

    return vomsesPath;
  }

  bool UserConfig::LoadConfigurationFile(const std::string& conffile, bool ignoreJobListFile) {
    if (!conffile.empty()) {
      IniConfig ini(conffile);
      if (ini) {
        logger.msg(INFO, "Loading configuration (%s)", conffile);

        // Legacy alias support
        if (ini["alias"]) {
          XMLNode alias = ini["alias"];
          while (alias.Child()) {
            std::string group = alias.Child().Name();
            std::list<std::string> serviceStrings;
            tokenize(alias.Child(), serviceStrings, " \t");
            for (std::list<std::string>::iterator it = serviceStrings.begin(); it != serviceStrings.end(); it++) {
              std::list<ConfigEndpoint> services = GetServices(*it);
              if (services.empty()) {
                ConfigEndpoint service = ServiceFromLegacyString(*it);
                if (service) {
                  services.push_back(service);
                }
              }
              groupMap[group].insert(groupMap[group].end(), services.begin(), services.end());
            }
            alias.Child().Destroy();
          }
        }

        if (ini["common"]) {
          XMLNode common = ini["common"];
          HANDLESTRATT("verbosity", Verbosity)
          if (!ignoreJobListFile) {
            HANDLESTRATT("joblist", JobListFile)
          }
          if (common["timeout"]) {
            if (!stringto(common["timeout"], timeout))
              logger.msg(WARNING, "The value of the timeout attribute in the configuration file (%s) was only partially parsed", conffile);
            common["timeout"].Destroy();
            if (common["timeout"]) {
              logger.msg(WARNING, "Multiple %s attributes in configuration file (%s)", "timeout", conffile);
              while (common["timeout"]) common["timeout"].Destroy();
            }
          }
          if (common["brokername"]) {
            broker = std::make_pair<std::string, std::string>(ini["common"]["brokername"],
                                                              ini["common"]["brokerarguments"] ? ini["common"]["brokerarguments"] : "");
            common["brokername"].Destroy();
            if (common["brokername"]) {
              logger.msg(WARNING, "Multiple %s attributes in configuration file (%s)", "brokername", conffile);
              while (common["brokername"]) common["brokername"].Destroy();
            }
            if (common["brokerarguments"]) {
              common["brokerarguments"].Destroy();
              if (common["brokerarguments"]) {
                logger.msg(WARNING, "Multiple %s attributes in configuration file (%s)", "brokerarguments", conffile);
                while (common["brokerarguments"]) common["brokerarguments"].Destroy();
              }
            }
          }
          // This block must be executed after the 'brokername' block.
          if (common["brokerarguments"]) {
            logger.msg(WARNING, "The brokerarguments attribute can only be used in conjunction with the brokername attribute");
            while (common["brokerarguments"]) common["brokerarguments"].Destroy();
          }
          if (common["bartender"]) {
            std::list<std::string> bartendersStr;
            tokenize(common["bartender"], bartendersStr, " \t");
            for (std::list<std::string>::const_iterator it = bartendersStr.begin();
                 it != bartendersStr.end(); it++) {
              URL bartenderURL(*it);
              if (!bartenderURL)
                logger.msg(WARNING, "Could not convert the bartender attribute value (%s) to an URL instance in configuration file (%s)", *it, conffile);
              else
                bartenders.push_back(bartenderURL);
            }
            common["bartender"].Destroy();
            if (common["bartender"]) {
              logger.msg(WARNING, "Multiple %s attributes in configuration file (%s)", "bartender", conffile);
              while (common["bartender"]) common["bartender"].Destroy();
            }
          }
          HANDLESTRATT("vomsespath", VOMSESPath)
          HANDLESTRATT("username", UserName)
          HANDLESTRATT("password", Password)
          HANDLESTRATT("proxypath", ProxyPath)
          HANDLESTRATT("certificatepath", CertificatePath)
          HANDLESTRATT("keypath", KeyPath)
          HANDLESTRATT("keypassword", KeyPassword)
          if (common["keysize"]) {
            if (!stringto(ini["common"]["keysize"], keySize))
              logger.msg(WARNING, "The value of the keysize attribute in the configuration file (%s) was only partially parsed", conffile);
            common["keysize"].Destroy();
            if (common["keysize"]) {
              logger.msg(WARNING, "Multiple %s attributes in configuration file (%s)", "keysize", conffile);
              while (common["keysize"]) common["keysize"].Destroy();
            }
          }
          HANDLESTRATT("cacertificatepath", CACertificatePath)
          HANDLESTRATT("cacertificatesdirectory", CACertificatesDirectory)
          if (common["certificatelifetime"]) {
            certificateLifeTime = Period((std::string)common["certificatelifetime"]);
            common["certificatelifetime"].Destroy();
            if (common["certificatelifetime"]) {
              logger.msg(WARNING, "Multiple %s attributes in configuration file (%s)", "certificatelifetime", conffile);
              while (common["certificatelifetime"]) common["certificatelifetime"].Destroy();
            }
          }
          if (common["slcs"]) {
            slcs = URL((std::string)common["slcs"]);
            if (!slcs) {
              logger.msg(WARNING, "Could not convert the slcs attribute value (%s) to an URL instance in configuration file (%s)", (std::string)common["slcs"], conffile);
              slcs = URL();
            }
            common["slcs"].Destroy();
            if (common["slcs"]) {
              logger.msg(WARNING, "Multiple %s attributes in configuration file (%s)", "slcs", conffile);
              while (common["slcs"]) common["slcs"].Destroy();
            }
          }
          HANDLESTRATT("jobdownloaddirectory", JobDownloadDirectory)
          HANDLESTRATT("storedirectory", StoreDirectory)
          HANDLESTRATT("idpname", IdPName)
          HANDLESTRATT("infointerface", InfoInterface)
          HANDLESTRATT("submissioninterface", SubmissionInterface)
          // Legacy defaultservices support
          if (common["defaultservices"]) {
            std::list<std::string> defaultServicesStr;
            tokenize(common["defaultservices"], defaultServicesStr, " \t");
            for (std::list<std::string>::const_iterator it = defaultServicesStr.begin(); it != defaultServicesStr.end(); it++) {
              ConfigEndpoint service = ServiceFromLegacyString(*it);
              if (service) defaultServices.push_back(service);
            }
            common["defaultservices"].Destroy();
            if (common["defaultservices"]) {
              logger.msg(WARNING,
                         "Multiple %s attributes in configuration file (%s)",
                         "defaultservices", conffile);
              while (common["defaultservices"])
                common["defaultservices"].Destroy();
            }
          }
          
          while (common["rejectdiscovery"]) {
            rejectDiscoveryURLs.push_back((std::string)common["rejectdiscovery"]);
            common["rejectdiscovery"].Destroy();
          }

          while (common["rejectmanagement"]) {
            rejectManagementURLs.push_back((std::string)common["rejectmanagement"]);
            common["rejectmanagement"].Destroy();
          }
    
          HANDLESTRATT("overlayfile", OverlayFile)
          if(!overlayfile.empty())
            if (!Glib::file_test(overlayfile, Glib::FILE_TEST_IS_REGULAR))
              logger.msg(WARNING,
                         "Specified overlay file (%s) does not exist.",
                         overlayfile);
          while (common.Child()) {
            logger.msg(WARNING,
                       "Unknown attribute %s in common section, ignoring it",
                       common.Child().Name());
            common.Child().Destroy();
          }

          common.Destroy();
        }

        const std::string registrySectionPrefix = "registry/";
        const std::string computingSectionPrefix = "computing/";
        while (XMLNode section = ini.Child()) {
          std::string sectionName = section.Name();
          if (sectionName.find(registrySectionPrefix) == 0 || sectionName.find(computingSectionPrefix) == 0) {
            ConfigEndpoint service(section["url"]);
            std::string alias;
            if (sectionName.find(registrySectionPrefix) == 0) {
              alias = sectionName.substr(registrySectionPrefix.length());
              service.type = ConfigEndpoint::REGISTRY;
              service.InterfaceName = (std::string)section["registryinterface"];
            } else {
              alias = sectionName.substr(computingSectionPrefix.length());
              service.type = ConfigEndpoint::COMPUTINGINFO;
              service.InterfaceName = (std::string)section["infointerface"];
              if (service.InterfaceName.empty()) {
                service.InterfaceName = InfoInterface();
              }
              service.RequestedSubmissionInterfaceName = (std::string)section["submissioninterface"];
              if (service.RequestedSubmissionInterfaceName.empty()) {
                service.RequestedSubmissionInterfaceName = SubmissionInterface();
              }
            }
              
            allServices[alias] = service;
            if (section["default"] && section["default"] != "no") {
              defaultServices.push_back(service);
            }
            while (section["group"]) {
              groupMap[section["group"]].push_back(service);
              section["group"].Destroy();
            }
          } else {
            logger.msg(INFO, "Unknown section %s, ignoring it", sectionName);
          }
          section.Destroy();
        }
        logger.msg(INFO, "Configuration (%s) loaded", conffile);
      }
      else
        logger.msg(WARNING, "Could not load configuration (%s)", conffile);
    }

    return true;
  }

  bool UserConfig::SaveToFile(const std::string& filename) const {
    std::ofstream file(filename.c_str(), std::ios::out);
    if (!file)
      return false;

    file << "[common]" << std::endl;
    if (!proxyPath.empty())
      file << "proxypath = " << proxyPath << std::endl;
    if (!certificatePath.empty())
      file << "certificatepath = " << certificatePath << std::endl;
    if (!keyPath.empty())
      file << "keypath = " << keyPath << std::endl;
    if (!keyPassword.empty())
      file << "keypassword = " << keyPassword << std::endl;
    if (keySize > 0)
      file << "keysize = " << keySize << std::endl;
    if (!caCertificatePath.empty())
      file << "cacertificatepath = " << caCertificatePath << std::endl;
    if (!caCertificatesDirectory.empty())
      file << "cacertificatesdirectory = " << caCertificatesDirectory << std::endl;
    if (certificateLifeTime > 0)
      file << "certificatelifetime = " << certificateLifeTime << std::endl;
    if (slcs)
      file << "slcs = " << slcs.fullstr() << std::endl;
    if (!rejectDiscoveryURLs.empty()) {
      for (std::list<std::string>::const_iterator it = rejectDiscoveryURLs.begin(); it != rejectDiscoveryURLs.end(); it++) {
        file << "rejectdiscovery = " << (*it) << std::endl;
      }
    }
    if (!rejectManagementURLs.empty()) {
      for (std::list<std::string>::const_iterator it = rejectManagementURLs.begin(); it != rejectManagementURLs.end(); it++) {
        file << "rejectmanagement = " << (*it) << std::endl;
      }
    }
    if (!verbosity.empty())
      file << "verbosity = " << verbosity << std::endl;
    if (!joblistfile.empty())
      file << "joblist = " << joblistfile << std::endl;
    if (timeout > 0)
      file << "timeout = " << timeout << std::endl;
    if (!broker.first.empty()) {
      file << "brokername = " << broker.first << std::endl;
      if (!broker.second.empty())
        file << "brokerarguments = " << broker.second << std::endl;
    }
    if (!bartenders.empty()) {
      file << "bartender =";
      for (std::vector<URL>::const_iterator it = bartenders.begin();
           it != bartenders.end(); it++) {
        file << " " << it->fullstr();
      }
      file << std::endl;
    }
    if (!vomsesPath.empty())
      file << "vomsespath = " << vomsesPath << std::endl;
    if (!username.empty())
      file << "username = " << username << std::endl;
    if (!password.empty())
      file << "password = " << password << std::endl;
    if (!storeDirectory.empty())
      file << "storedirectory = " << storeDirectory << std::endl;
    if (!idPName.empty())
      file << "idpname = " << idPName << std::endl;
    if (!overlayfile.empty())
      file << "overlayfile = " << overlayfile << std::endl;
    if (!submissioninterface.empty())
      file << "submissioninterface = " << submissioninterface << std::endl;
    if (!infointerface.empty())
      file << "infointerface = " << infointerface << std::endl;
    
    for (std::map<std::string, ConfigEndpoint>::const_iterator it = allServices.begin(); it != allServices.end(); it++) {
      if (it->second.type == ConfigEndpoint::REGISTRY) {
        file << "[registry/" << it->first << "]" << std::endl;
        file << "url = " << it->second.URLString << std::endl;
        if (!it->second.InterfaceName.empty()) {
          file << "registryinterface = " << it->second.InterfaceName << std::endl;
        }
      } else {
        file << "[computing/" << it->first << "]" << std::endl;                  
        file << "url = " << it->second.URLString << std::endl;
        if (!it->second.InterfaceName.empty()) {
          file << "infointerface = " << it->second.InterfaceName << std::endl;
        }
        if (!it->second.RequestedSubmissionInterfaceName.empty()) {
          file << "submissioninterface = " << it->second.RequestedSubmissionInterfaceName << std::endl;
        }
      }
      if (std::find(defaultServices.begin(), defaultServices.end(), it->second) != defaultServices.end()) {
        file << "default = yes" << std::endl;
      }
      for (std::map<std::string, std::list<ConfigEndpoint> >::const_iterator git = groupMap.begin(); git != groupMap.end(); git++) {
        if (std::find(git->second.begin(), git->second.end(), it->second) != git->second.end()) {
          file << "group = " << git->first << std::endl;
        }
      }
    }
    
    logger.msg(INFO, "UserConfiguration saved to file (%s)", filename);

    return true;
  }

  bool UserConfig::CreateDefaultConfigurationFile() const {
    // If the default configuration file does not exist, copy an example file.
    if (!Glib::file_test(DEFAULTCONFIG, Glib::FILE_TEST_EXISTS)) {

      // Check if the parent directory exist.
      if (!Glib::file_test(ARCUSERDIRECTORY, Glib::FILE_TEST_EXISTS)) {
        // Create directory.
        if (!makeDir(ARCUSERDIRECTORY)) {
          logger.msg(WARNING, "Unable to create %s directory.", ARCUSERDIRECTORY);
          return false;
        }
      }

      if (dir_test(ARCUSERDIRECTORY)) {
        if (Glib::file_test(EXAMPLECONFIG, Glib::FILE_TEST_IS_REGULAR)) {
          // Destination: Get basename, remove example prefix and add .arc directory.
          if (copyFile(EXAMPLECONFIG, DEFAULTCONFIG))
            logger.msg(VERBOSE, "Configuration example file created (%s)", DEFAULTCONFIG);
          else
            logger.msg(INFO, "Unable to copy example configuration from existing configuration (%s)", EXAMPLECONFIG);
            return false;
        }
        else {
          logger.msg(INFO, "Cannot copy example configuration (%s), it is not a regular file", EXAMPLECONFIG);
          return false;
        }
      }
      else {
        logger.msg(INFO, "Example configuration (%s) not created.", DEFAULTCONFIG);
        return false;
      }
    }
    else if (!Glib::file_test(DEFAULTCONFIG, Glib::FILE_TEST_IS_REGULAR)) {
      logger.msg(INFO, "The default configuration file (%s) is not a regular file.", DEFAULTCONFIG);
      return false;
    }

    return true;
  }

  void UserConfig::setDefaults() {
    timeout = DEFAULT_TIMEOUT;
    broker.first = DEFAULT_BROKER;
    broker.second = "";
  }

  bool UserConfig::makeDir(const std::string& path) {
    bool dirCreated = false;
    dirCreated = DirCreate(path, 0755);

    if (dirCreated)
      logger.msg(INFO, "%s directory created", path);
    else
      logger.msg(WARNING, "Failed to create directory %s", path);

    return dirCreated;
  }

  bool UserConfig::copyFile(const std::string& source, const std::string& destination) {
/*
TODO: Make FileUtils function to this
#ifdef HAVE_GIOMM
    try {
      return Gio::File::create_for_path(source)->copy(Gio::File::create_for_path(destination), Gio::FILE_COPY_NONE);
    } catch (Gio::Error e) {
      logger.msg(WARNING, "%s", (std::string)e.what());
      return false;
    }
#else
*/
    std::ifstream ifsSource(source.c_str(), std::ios::in | std::ios::binary);
    if (!ifsSource)
      return false;

    std::ofstream ofsDestination(destination.c_str(), std::ios::out | std::ios::binary);
    if (!ofsDestination)
      return false;

    int bytesRead = -1;
    char buff[1024];
    while (bytesRead != 0) {
      ifsSource.read((char*)buff, 1024);
      bytesRead = ifsSource.gcount();
      ofsDestination.write((char*)buff, bytesRead);
    }

    return true;
//#endif
  }

  bool UserConfig::UtilsDirPath(const std::string& dir) {
    if (!DirCreate(dir, 0700, true))
      logger.msg(WARNING, "Failed to create directory %s", dir);
    else
      utilsdir = dir;
    return true;
  }

  std::list<ConfigEndpoint> UserConfig::FilterServices(const std::list<ConfigEndpoint>& unfilteredServices, ConfigEndpoint::Type type) {
    std::list<ConfigEndpoint> services;
    for (std::list<ConfigEndpoint>::const_iterator it = unfilteredServices.begin(); it != unfilteredServices.end(); it++) {
      if (type == ConfigEndpoint::ANY || type == it->type) {
        services.push_back(*it);
      }
    }
    return services;
  }
  
  std::list<ConfigEndpoint> UserConfig::GetDefaultServices(ConfigEndpoint::Type type) {
    return FilterServices(defaultServices, type);
  }
  
  ConfigEndpoint UserConfig::GetService(const std::string& alias) {
    return allServices[alias];
  }

  std::list<ConfigEndpoint> UserConfig::GetServices(const std::string& groupOrAlias, ConfigEndpoint::Type type) {
    std::list<ConfigEndpoint> services = GetServicesInGroup(groupOrAlias);
    if (services.empty()) {
      ConfigEndpoint service = GetService(groupOrAlias);
      if (service) services.push_back(service);
    }
    return FilterServices(services, type);
  }
  

  std::list<ConfigEndpoint> UserConfig::GetServicesInGroup(const std::string& group, ConfigEndpoint::Type type) {
    return FilterServices(groupMap[group], type);
  }

  ConfigEndpoint UserConfig::ServiceFromLegacyString(std::string type_flavour_url) {
    ConfigEndpoint service;
    size_t pos = type_flavour_url.find(":");
    if (pos != type_flavour_url.npos) {
      std::string type = type_flavour_url.substr(0, pos);
      std::string flavour_url = type_flavour_url.substr(pos + 1);
      if (type == "index") {
        service.type = ConfigEndpoint::REGISTRY;
      } else if (type == "computing") {
        service.type = ConfigEndpoint::COMPUTINGINFO;
      } else {
        return service;
      }
      pos = flavour_url.find(":");
      if (pos != flavour_url.npos) {
        std::string flavour = flavour_url.substr(0, pos);
        std::string url = flavour_url.substr(pos + 1);
        if (service.type == ConfigEndpoint::REGISTRY) {
          std::string registryinterface = "org.nordugrid.emir";
          if (flavour == "ARC0") registryinterface = "org.nordugrid.ldapegiis";
          service.InterfaceName = registryinterface;
        } else if (service.type == ConfigEndpoint::COMPUTINGINFO) {
          std::string infointerface = "org.nordugrid.ldapglue2";
          if (flavour == "ARC0") infointerface = "org.nordugrid.ldapng";
          if (flavour == "ARC1") infointerface = "org.nordugrid.wsrfglue2";
          if (flavour == "EMIES") infointerface = "org.ogf.emies";
          if (flavour == "CREAM") infointerface = "org.nordugrid.ldapglue1";
          service.InterfaceName = infointerface;
        }
        service.URLString = url;
      }
    }
    return service;
  }

  

static std::string cert_file_fix(const std::string& old_file,std::string& new_file) {
  struct stat st;
  if(old_file.empty()) return old_file;
  if(::stat(old_file.c_str(),&st) != 0) return old_file;
// No getuid on win32
#ifndef WIN32
  if(::getuid() == st.st_uid) return old_file;
#endif
  std::string tmpname = Glib::build_filename(Glib::get_tmp_dir(), "arccred.XXXXXX");
  int tmph = Glib::mkstemp(tmpname);
  if(tmph == -1) return old_file;
  int oldh = ::open(old_file.c_str(),O_RDONLY);
  if(oldh == -1) {
    ::close(tmph); ::unlink(tmpname.c_str());
    return old_file;
  };
  if(!FileCopy(oldh,tmph)) {
    ::close(tmph); ::unlink(tmpname.c_str());
    ::close(oldh);
    return old_file;
  };
  ::close(tmph);
  ::close(oldh);
  new_file = tmpname;
  return new_file;
}

#define GET_OLD_VAR(name,var,set) { \
  var = GetEnv(name,set); \
}

#define SET_OLD_VAR(name,var,set) { \
  if(set) { \
    SetEnv(name,var,true); \
  } else { \
    UnsetEnv(name); \
  }; \
}

#define SET_NEW_VAR(name,val) { \
  std::string v = val; \
  if(v.empty()) { \
    UnsetEnv(name); \
  } else { \
    SetEnv(name,v,true); \
  }; \
}

#define SET_NEW_VAR_FILE(name,val,fname) { \
  std::string v = val; \
  if(v.empty()) { \
    UnsetEnv(name); \
  } else { \
    v = cert_file_fix(v,fname); \
    SetEnv(name,v,true); \
  }; \
}

  CertEnvLocker::CertEnvLocker(const UserConfig& cfg) {
    EnvLockAcquire();
    GET_OLD_VAR("X509_USER_KEY",x509_user_key_old,x509_user_key_set);
    GET_OLD_VAR("X509_USER_CERT",x509_user_cert_old,x509_user_cert_set);
    GET_OLD_VAR("X509_USER_PROXY",x509_user_proxy_old,x509_user_proxy_set);
    GET_OLD_VAR("X509_CERT_DIR",ca_cert_dir_old,ca_cert_dir_set);
    SET_NEW_VAR_FILE("X509_USER_KEY",cfg.KeyPath(),x509_user_key_new);
    SET_NEW_VAR_FILE("X509_USER_CERT",cfg.CertificatePath(),x509_user_cert_new);
    SET_NEW_VAR_FILE("X509_USER_PROXY",cfg.ProxyPath(),x509_user_proxy_new);
    SET_NEW_VAR("X509_CERT_DIR",cfg.CACertificatesDirectory());
    EnvLockWrap(false);
  }

  CertEnvLocker::~CertEnvLocker(void) {
    EnvLockUnwrap(false);
    SET_OLD_VAR("X509_CERT_DIR",ca_cert_dir_old,ca_cert_dir_set);
    SET_OLD_VAR("X509_USER_PROXY",x509_user_proxy_old,x509_user_proxy_set);
    SET_OLD_VAR("X509_USER_CERT",x509_user_cert_old,x509_user_cert_set);
    SET_OLD_VAR("X509_USER_KEY",x509_user_key_old,x509_user_key_set);
    if(!x509_user_key_new.empty()) ::unlink(x509_user_key_new.c_str());
    if(!x509_user_cert_new.empty()) ::unlink(x509_user_cert_new.c_str());
    if(!x509_user_proxy_new.empty()) ::unlink(x509_user_proxy_new.c_str());
    EnvLockRelease();
  }

} // namespace Arc
