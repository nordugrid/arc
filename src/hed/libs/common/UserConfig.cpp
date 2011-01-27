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

  UserConfig::UserConfig(initializeCredentialsType initializeCredentials) {
    if (initializeCredentials != initializeCredentialsType::SkipCredentials) {
      InitializeCredentials();
      if ((!CredentialsFound()) && (initializeCredentials == initializeCredentialsType::RequireCredentials))
        return;
    }
    else if (initializeCredentials == initializeCredentialsType::SkipCACredentials) {
      InitializeCredentials();
      if(proxyPath.empty() && (certificatePath.empty() || keyPath.empty()))
        return;
    }

    ok = true;

    setDefaults();
  }

  UserConfig::UserConfig(const std::string& conffile,
                         initializeCredentialsType initializeCredentials,
                         bool loadSysConfig)
    : ok(false) {
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

    if (initializeCredentials != initializeCredentialsType::SkipCredentials) {
      InitializeCredentials();
      if ((!CredentialsFound()) && (initializeCredentials == initializeCredentialsType::RequireCredentials))
        return;
    }
    else if (initializeCredentials == initializeCredentialsType::SkipCACredentials) {
      InitializeCredentials();
      if(proxyPath.empty() && (certificatePath.empty() || keyPath.empty()))
        return;
    }


    ok = true;

    setDefaults();
  }

  UserConfig::UserConfig(const std::string& conffile, const std::string& jfile,
                         initializeCredentialsType initializeCredentials, bool loadSysConfig)
    : ok(false) {
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

    if (initializeCredentials != initializeCredentialsType::SkipCredentials) {
      InitializeCredentials();
      if ((!CredentialsFound()) && (initializeCredentials == initializeCredentialsType::RequireCredentials))
        return;
    }
    else if (initializeCredentials == initializeCredentialsType::SkipCACredentials) {
      InitializeCredentials();
      if(proxyPath.empty() && (certificatePath.empty() || keyPath.empty()))
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
      else if (!Glib::file_test(joblistdir, Glib::FILE_TEST_IS_DIR)) {
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

  void UserConfig::InitializeCredentials() {
    const User user;
    // Look for credentials.
    if (!GetEnv("X509_USER_PROXY").empty()) {
      if (!Glib::file_test(proxyPath = GetEnv("X509_USER_PROXY"), Glib::FILE_TEST_IS_REGULAR)) {
        logger.msg(ERROR, "Can not access proxy file: %s", proxyPath);
        proxyPath.clear();
      }
    }
    else if (!GetEnv("X509_USER_CERT").empty() &&
             !GetEnv("X509_USER_KEY").empty()) {
      if (!Glib::file_test(certificatePath = GetEnv("X509_USER_CERT"), Glib::FILE_TEST_IS_REGULAR)) {
        logger.msg(ERROR, "Can not access certificate file: %s", certificatePath);
        certificatePath.clear();
      }
      if (!Glib::file_test(keyPath = GetEnv("X509_USER_KEY"), Glib::FILE_TEST_IS_REGULAR)) {
        logger.msg(ERROR, "Can not access key file: %s", keyPath);
        keyPath.clear();
      }
    }
    else if (!proxyPath.empty()) {
      if (!Glib::file_test(proxyPath, Glib::FILE_TEST_IS_REGULAR)) {
        logger.msg(ERROR, "Can not access proxy file: %s", proxyPath);
        proxyPath.clear();
      }
    }
    else if (!certificatePath.empty() && !keyPath.empty()) {
      if (!Glib::file_test(certificatePath.c_str(), Glib::FILE_TEST_IS_REGULAR)) {
        logger.msg(ERROR, "Can not access certificate file: %s", certificatePath);
        certificatePath.clear();
      }
      if (!Glib::file_test(keyPath.c_str(), Glib::FILE_TEST_IS_REGULAR)) {
        logger.msg(ERROR, "Can not access key file: %s", keyPath);
        keyPath.clear();
      }
    }
    else if (!Glib::file_test(proxyPath = Glib::build_filename(Glib::get_tmp_dir(), std::string("x509up_u") + tostring(user.get_uid())), Glib::FILE_TEST_IS_REGULAR)) {
      logger.msg(WARNING, "Default proxy file does not exist: %s "
                          "trying default certificate and key", proxyPath);
      proxyPath.clear();
      if (user.get_uid() == 0) {
        certificatePath = Glib::build_filename(G_DIR_SEPARATOR_S + std::string("etc"), std::string("grid-security") + G_DIR_SEPARATOR_S + std::string("hostcert.pem"));
        keyPath = Glib::build_filename(G_DIR_SEPARATOR_S + std::string("etc"), std::string("grid-security") + G_DIR_SEPARATOR_S + std::string("hostkey.pem"));
      }
      else {
        certificatePath = Glib::build_filename(user.Home(), std::string(".globus") + G_DIR_SEPARATOR_S +  std::string("usercert.pem"));
        keyPath = Glib::build_filename(user.Home(), std::string(".globus") + G_DIR_SEPARATOR_S + std::string("userkey.pem"));
      }
#ifdef WIN32
        certificatePath = Glib::build_filename(std::string(g_get_home_dir()), std::string(".globus") + G_DIR_SEPARATOR_S +  std::string("usercert.pem"));
        keyPath = Glib::build_filename(std::string(g_get_home_dir()), std::string(".globus") + G_DIR_SEPARATOR_S + std::string("userkey.pem"));
#endif

      if (!Glib::file_test(certificatePath, Glib::FILE_TEST_IS_REGULAR)) {
        logger.msg(ERROR, "Can not access certificate file: %s", certificatePath);
        certificatePath.clear();
      }
      if (!Glib::file_test(keyPath, Glib::FILE_TEST_IS_REGULAR)) {
        logger.msg(ERROR, "Can not access key file: %s", keyPath);
        keyPath.clear();
      }
    }

    if (!GetEnv("X509_CERT_DIR").empty()) {
      if (!Glib::file_test(caCertificatesDirectory = GetEnv("X509_CERT_DIR"), Glib::FILE_TEST_IS_DIR)) {
        logger.msg(WARNING, "Can not access CA certificate directory: %s. The certificates will not be verified.", caCertificatesDirectory);
        caCertificatesDirectory.clear();
      }
    }
    else if (!caCertificatesDirectory.empty()) {
      if (!Glib::file_test(caCertificatesDirectory, Glib::FILE_TEST_IS_DIR)) {
        logger.msg(WARNING, "Can not access CA certificate directory: %s. The certificates will not be verified", caCertificatesDirectory);
        caCertificatesDirectory.clear();
      }
    }
    else if ((user.get_uid() == 0 || !Glib::file_test(caCertificatesDirectory = user.Home() + G_DIR_SEPARATOR_S + ".globus" + G_DIR_SEPARATOR_S + "certificates", Glib::FILE_TEST_IS_DIR)) &&
             !Glib::file_test(caCertificatesDirectory = std::string(Glib::get_home_dir()) + G_DIR_SEPARATOR_S + ".globus" + G_DIR_SEPARATOR_S + "certificates", Glib::FILE_TEST_IS_DIR) &&
             !Glib::file_test(caCertificatesDirectory = ArcLocation::Get() + G_DIR_SEPARATOR_S + "etc" + G_DIR_SEPARATOR_S + "certificates", Glib::FILE_TEST_IS_DIR) &&
             !Glib::file_test(caCertificatesDirectory = ArcLocation::Get() + G_DIR_SEPARATOR_S + "etc" + G_DIR_SEPARATOR_S + "grid-security" + G_DIR_SEPARATOR_S + "certificates", Glib::FILE_TEST_IS_DIR) &&
             !Glib::file_test(caCertificatesDirectory = ArcLocation::Get() + G_DIR_SEPARATOR_S + "share" + G_DIR_SEPARATOR_S + "certificates", Glib::FILE_TEST_IS_DIR)) {
      caCertificatesDirectory = Glib::build_filename(G_DIR_SEPARATOR_S + std::string("etc"), std::string("grid-security") + G_DIR_SEPARATOR_S + std::string("certificates"));
      if (!Glib::file_test(caCertificatesDirectory.c_str(), Glib::FILE_TEST_IS_DIR)) {
        logger.msg(WARNING, "Can not locate CA certificate directory on the default directories: ~/.globus/certificates, $ARC_LOCATION/etc/certificates, $ARC_LOCATION/etc/grid-security/certificates, $ARC_LOCATION/share/certificates, /etc/grid-security/certificates. The certificate will not be verified.");
        caCertificatesDirectory.clear();
      }
    }

    if (!proxyPath.empty())
      logger.msg(INFO, "Using proxy file: %s", proxyPath);
    else if (!certificatePath.empty() && !keyPath.empty()) {
      logger.msg(INFO, "Using certificate file: %s", certificatePath);
      logger.msg(INFO, "Using key file: %s", keyPath);
    }

    if (!caCertificatesDirectory.empty())
      logger.msg(INFO, "Using CA certificate directory: %s", caCertificatesDirectory);
  }

  bool UserConfig::LoadConfigurationFile(const std::string& conffile, bool ignoreJobListFile) {
    if (!conffile.empty()) {
      IniConfig ini(conffile);
      if (ini) {
        logger.msg(INFO, "Loading configuration (%s)", conffile);

        if (ini["alias"]) {
          XMLNode aliasXML = ini["alias"];
          if (aliasMap) { // Merge aliases. Overwrite existing aliases
            while (aliasXML.Child()) {
              if (aliasMap[aliasXML.Child().Name()]) {
                aliasMap[aliasXML.Child().Name()].Replace(aliasXML.Child());
                logger.msg(INFO, "Overwriting already defined alias \"%s\"",  aliasXML.Child().Name());
              }
              else
                aliasMap.NewChild(aliasXML.Child());

              aliasXML.Child().Destroy();
            }
          }
          else
            aliasXML.New(aliasMap);
          aliasXML.Destroy();
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
          HANDLESTRATT("vomsserverpath", VOMSServerPath)
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
          HANDLESTRATT("storedirectory", StoreDirectory)
          HANDLESTRATT("idpname", IdPName)
          if (common["defaultservices"]) {
            if (!selectedServices[COMPUTING].empty() ||
                !selectedServices[INDEX].empty())
              ClearSelectedServices();
            std::list<std::string> defaultServicesStr;
            tokenize(common["defaultservices"], defaultServicesStr, " \t");
            for (std::list<std::string>::const_iterator it =
                   defaultServicesStr.begin();
                 it != defaultServicesStr.end(); it++) {
              // Aliases cannot contain '.' or ':'
              if (it->find_first_of(":.") == std::string::npos) { // Alias
                if (!aliasMap[*it]) {
                  logger.msg(ERROR, "Could not resolve alias \"%s\" it is not defined.", *it);
                  return false;
                }

                std::list<std::string> resolvedAliases(1, *it);
                if (!ResolveAlias(selectedServices, resolvedAliases))
                  return false;
              }
              else {
                const std::size_t pos1 = it->find(":");
                if (pos1 == std::string::npos) {
                  logger.msg(WARNING,
                             "The defaultservices attribute value contains a "
                             "wrongly formated element (%s) in configuration "
                             "file (%s)", *it, conffile);
                  continue;
                }
                const std::string serviceType = it->substr(0, pos1);
                if (serviceType != "computing" && serviceType != "index") {
                  logger.msg(WARNING,
                             "The defaultservices attribute value contains an "
                             "unknown servicetype %s at %s in configuration "
                             "file (%s)", serviceType, *it, conffile);
                  continue;
                }
                const std::string service = it->substr(pos1 + 1);
                logger.msg(VERBOSE, "Adding selected service %s:%s",
                           serviceType, service);
                if (serviceType == "computing")
                  selectedServices[COMPUTING].push_back(service);
                else
                  selectedServices[INDEX].push_back(service);
              }
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
          if (common["rejectservices"]) {
            std::list<std::string> rejectServicesStr;
            tokenize(common["rejectservices"], rejectServicesStr, " \t");
            for (std::list<std::string>::const_iterator it =
                   rejectServicesStr.begin();
                 it != rejectServicesStr.end(); it++) {
              // Aliases cannot contain '.' or ':'.
              if (it->find_first_of(":.") == std::string::npos) { // Alias
                if (!aliasMap[*it]) {
                  logger.msg(ERROR, "Could not resolve alias \"%s\" it is not defined.", *it);
                  return false;
                }

                std::list<std::string> resolvedAliases(1, *it);
                if (!ResolveAlias(selectedServices, resolvedAliases))
                  return false;
              }
              else {
                const std::size_t pos1 = it->find(":");
                if (pos1 == std::string::npos) {
                  logger.msg(WARNING,
                             "The rejectservices attribute value contains a "
                             "wrongly formated element (%s) in configuration "
                             "file (%s)", *it, conffile);
                  continue;
                }
                const std::string serviceType = it->substr(0, pos1);
                if (serviceType != "computing" && serviceType != "index") {
                  logger.msg(WARNING,
                             "The rejectservices attribute value contains an "
                             "unknown servicetype %s at %s in configuration "
                             "file (%s)", serviceType, *it, conffile);
                  continue;
                }
                const std::string service = it->substr(pos1 + 1);
                logger.msg(VERBOSE, "Adding rejected service %s:%s",
                           serviceType, service);
                if (serviceType == "computing")
                  rejectedServices[COMPUTING].push_back(service);
                else
                  rejectedServices[INDEX].push_back(service);
              }
            }
            common["rejectservices"].Destroy();
            if (common["rejectservices"]) {
              logger.msg(WARNING,
                         "Multiple %s attributes in configuration file (%s)",
                         "rejectservices", conffile);
              while (common["rejectservices"])
                common["rejectservices"].Destroy();
            }
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

        while (ini.Child()) {
          logger.msg(INFO, "Unknown section %s, ignoring it",
                     ini.Child().Name());
          ini.Child().Destroy();
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

    file << "[ common ]" << std::endl;
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
    if (!vomsServerPath.empty())
      file << "vomsserverpath = " << vomsServerPath << std::endl;
    if (!username.empty())
      file << "username = " << username << std::endl;
    if (!password.empty())
      file << "password = " << password << std::endl;
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
    if (!storeDirectory.empty())
      file << "storedirectory = " << storeDirectory << std::endl;
    if (!idPName.empty())
      file << "idpname = " << idPName << std::endl;
    if (!selectedServices[COMPUTING].empty() ||
        !selectedServices[INDEX].empty()) {
      file << "defaultservices =";
      for (std::list<std::string>::const_iterator it =
             selectedServices[COMPUTING].begin();
           it != selectedServices[COMPUTING].end(); it++)
        file << " computing:" << *it;
      for (std::list<std::string>::const_iterator it =
             selectedServices[INDEX].begin();
           it != selectedServices[INDEX].end(); it++)
        file << " index:" << *it;
      file << std::endl;
    }
    if (!rejectedServices[COMPUTING].empty() ||
        !rejectedServices[INDEX].empty()) {
      file << "rejectedservices =";
      for (std::list<std::string>::const_iterator it =
             rejectedServices[COMPUTING].begin();
           it != rejectedServices[COMPUTING].end(); it++)
        file << " computing:" << *it;
      for (std::list<std::string>::const_iterator it =
             rejectedServices[INDEX].begin();
           it != rejectedServices[INDEX].end(); it++)
        file << " index:" << *it;
      file << std::endl;
    }
    if (!overlayfile.empty())
      file << "overlayfile = " << overlayfile << std::endl;

    if (aliasMap.Size() > 0) {
      int i = 0;
      file << std::endl << "[ alias ]" << std::endl;
      while (XMLNode n = const_cast<XMLNode&>(aliasMap).Child(i++)) {
        file << n.Name() << " = " << (std::string)n << std::endl;
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

      if (Glib::file_test(ARCUSERDIRECTORY, Glib::FILE_TEST_IS_DIR)) {
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

  bool UserConfig::AddServices(const std::list<std::string>& services,
                               ServiceType st) {
    const std::string serviceType = tostring(st);
    for (std::list<std::string>::const_iterator it = services.begin();
         it != services.end(); it++) {
      std::list<std::string>& servicesRef = ((*it)[0] != '-' ?
                                             selectedServices[st] :
                                             rejectedServices[st]);

      std::string service = ((*it)[0] != '-' ? *it : it->substr(1));

      // Alias cannot contain '.', ':', ' ' or '\t' and a URL must contain atleast one of ':' or '.'.
      if (service.find_first_of(":. \t") == std::string::npos) { // Alias.
        if (!aliasMap[service]) {
          logger.msg(ERROR, "Could not resolve alias \"%s\" it is not defined.", service);
          return false;
        }

        std::list<std::string> resolvedAliases(1, service);
        if (!ResolveAlias(servicesRef, st, resolvedAliases))
          return false;
      }
      else { // URL
        const std::size_t pos = service.find(":");
        const std::size_t pos2 = service.find("://");
        if (pos == std::string::npos || pos == pos2)
          service = "*:" + service;
        else if (pos == 0)
          service = "*" + service;
        logger.msg(VERBOSE, "Adding %s service %s", (*it)[0] != '-' ?
                   istring("selected") : istring("rejected"), service);
        servicesRef.push_back(service);
      }
    }

    return true;
  }

  bool UserConfig::AddServices(const std::list<std::string>& selected,
                               const std::list<std::string>& /* rejected */,
                               ServiceType st) {
    bool isSelectedNotRejected = true;
    const std::string serviceType = tostring(st);
    for (std::list<std::string>::const_iterator it = selected.begin();
         it != selected.end(); it++) {
      if (it == selected.end()) {
        if (selected.empty())
          break;
        it = selected.begin();
        isSelectedNotRejected = false;
      }

      std::list<std::string>& servicesRef = (isSelectedNotRejected ?
                                             selectedServices[st] :
                                             rejectedServices[st]);
      std::string service = *it;

      // Alias cannot contain '.', ':', ' ' or '\t' and a URL must contain atleast one of ':' or '.'.
      if (service.find_first_of(":. \t") == std::string::npos) { // Alias.
        if (!aliasMap[service]) {
          logger.msg(ERROR, "Could not resolve alias \"%s\" it is not defined.", service);
          return false;
        }

        std::list<std::string> resolvedAliases(1, service);
        if (!ResolveAlias(servicesRef, st, resolvedAliases))
          return false;
      }
      else { // URL
        const std::size_t pos = service.find(":");
        const std::size_t pos2 = service.find("://");
        if (pos == std::string::npos || pos == pos2)
          service = "*:" + service;
        else if (pos == 0)
          service = "*" + service;
        logger.msg(VERBOSE, "Adding %s service %s ", isSelectedNotRejected ?
                   istring("selected") : istring("rejected"), service);
        servicesRef.push_back(service);
      }
    }

    return true;
  }

  const std::list<std::string>& UserConfig::GetSelectedServices(ServiceType st) const {
    return selectedServices[st];
  }

  const std::list<std::string>& UserConfig::GetRejectedServices(ServiceType st) const {
    return rejectedServices[st];
  }

  void UserConfig::ClearSelectedServices(ServiceType st) {
    selectedServices[st].clear();
  }

  void UserConfig::ClearSelectedServices() {
    selectedServices[COMPUTING].clear();
    selectedServices[INDEX].clear();
  }

  void UserConfig::ClearRejectedServices(ServiceType st) {
    rejectedServices[st].clear();
  }

  void UserConfig::ClearRejectedServices() {
    rejectedServices[COMPUTING].clear();
    rejectedServices[INDEX].clear();
  }

  bool UserConfig::ResolveAlias(std::list<std::string>& services,
                                ServiceType st,
                                std::list<std::string>& resolvedAliases) {
    std::list<std::string> valueList;
    tokenize(aliasMap[resolvedAliases.back()], valueList, " \t");

    for (std::list<std::string>::const_iterator it = valueList.begin();
         it != valueList.end(); it++) {
      const std::size_t pos1 = it->find(":");
      const std::size_t pos2 = it->find(":", pos1 + 1);

      if (pos1 == std::string::npos) { // Alias.
        const std::string& referedAlias = *it;
        if (std::find(resolvedAliases.begin(), resolvedAliases.end(),
                      referedAlias) != resolvedAliases.end()) {
          // Loop detected.
          std::string loopstr = "";
          for (std::list<std::string>::const_iterator itloop =
                 resolvedAliases.begin();
               itloop != resolvedAliases.end(); itloop++)
            loopstr += *itloop + " -> ";
          loopstr += referedAlias;
          logger.msg(ERROR, "Cannot resolve alias \"%s\". Loop detected: %s",
                     resolvedAliases.front(), loopstr);
          return false;
        }

        if (!aliasMap[referedAlias]) {
          logger.msg(ERROR, "Cannot resolve alias %s, it is not defined",
                     referedAlias);
          return false;
        }

        resolvedAliases.push_back(referedAlias);
        if (!ResolveAlias(services, st, resolvedAliases))
          return false;

        resolvedAliases.pop_back();
      }
      else if (pos2 != std::string::npos) { // serviceType:flavour:URL
        const std::string serviceType = it->substr(0, pos1);
        if (serviceType != "computing" && serviceType != "index") {
          logger.msg(WARNING,
                     "Alias name (%s) contains a unknown servicetype %s at %s",
                     resolvedAliases.front(), serviceType, *it);
          continue;
        }
        else if ((st == COMPUTING && serviceType != "computing") ||
                 (st == INDEX && serviceType != "index"))
          continue;

        std::string service = it->substr(pos1 + 1);
        logger.msg(VERBOSE, "Adding service %s:%s from resolved alias %s",
                   serviceType, service, resolvedAliases.back());
        services.push_back(service);
      }
      else {
        logger.msg(WARNING,
                   "Alias (%s) contains a wrongly formatted element (%s)",
                   resolvedAliases.front(), *it);
      }
    }

    return true;
  }

  bool UserConfig::ResolveAlias(ServiceList& services,
                                std::list<std::string>& resolvedAliases) {
    std::list<std::string> valueList;
    tokenize(aliasMap[resolvedAliases.back()], valueList, " \t");

    for (std::list<std::string>::const_iterator it = valueList.begin();
         it != valueList.end(); it++) {
      const std::size_t pos1 = it->find(":");
      const std::size_t pos2 = it->find(":", pos1+1);

      if (pos1 == std::string::npos) { // Alias.
        const std::string& referedAlias = *it;
        if (std::find(resolvedAliases.begin(), resolvedAliases.end(),
                      referedAlias) != resolvedAliases.end()) {
          // Loop detected.
          std::string loopstr = "";
          for (std::list<std::string>::const_iterator itloop =
                 resolvedAliases.begin();
               itloop != resolvedAliases.end(); itloop++)
            loopstr += *itloop + " -> ";
          loopstr += referedAlias;
          logger.msg(ERROR, "Cannot resolve alias \"%s\". Loop detected: %s",
                     resolvedAliases.front(), loopstr);
          return false;
        }

        if (!aliasMap[referedAlias]) {
          logger.msg(ERROR, "Cannot resolve alias %s, it is not defined",
                     referedAlias);
          return false;
        }

        resolvedAliases.push_back(referedAlias);
        if (!ResolveAlias(services, resolvedAliases))
          return false;

        resolvedAliases.pop_back();
      }
      else if (pos2 != std::string::npos) { // serviceType:flavour:URL
        const std::string serviceType = it->substr(0, pos1);
        if (serviceType != "computing" && serviceType != "index") {
          logger.msg(WARNING,
                     "Alias name (%s) contains a unknown servicetype %s at %s",
                     resolvedAliases.front(), serviceType, *it);
          continue;
        }
        const std::string service = it->substr(pos1 + 1);
        logger.msg(VERBOSE, "Adding service %s:%s from resolved alias %s",
                   serviceType, service, resolvedAliases.back());
        if (serviceType == "computing")
          services[COMPUTING].push_back(service);
        else
          services[INDEX].push_back(service);
      }
      else {
        logger.msg(WARNING,
                   "Alias (%s) contains a wrongly formated element (%s)",
                   resolvedAliases.front(), *it);
      }
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
    SetEnvNonLock(name,var,true); \
  } else { \
    UnsetEnvNonLock(name); \
  }; \
}

#define SET_NEW_VAR(name,val) { \
  std::string v = val; \
  if(v.empty()) { \
    UnsetEnvNonLock(name); \
  } else { \
    SetEnvNonLock(name,v,true); \
  }; \
}

#define SET_NEW_VAR_FILE(name,val,fname) { \
  std::string v = val; \
  if(v.empty()) { \
    UnsetEnvNonLock(name); \
  } else { \
    v = cert_file_fix(v,fname); \
    SetEnvNonLock(name,v,true); \
  }; \
}

  CertEnvLocker::CertEnvLocker(const UserConfig& cfg) {
    EnvLockAcquire();
    GET_OLD_VAR("X509_USER_KEY",x509_user_key_old,x509_user_key_set);
    GET_OLD_VAR("X509_USER_CERT",x509_user_cert_old,x509_user_cert_set);
    GET_OLD_VAR("X509_USER_PROXY",x509_user_proxy_old,x509_user_proxy_set);
    GET_OLD_VAR("CA_CERT_DIR",ca_cert_dir_old,ca_cert_dir_set);
    SET_NEW_VAR_FILE("X509_USER_KEY",cfg.KeyPath(),x509_user_key_new);
    SET_NEW_VAR_FILE("X509_USER_CERT",cfg.CertificatePath(),x509_user_cert_new);
    SET_NEW_VAR_FILE("X509_USER_PROXY",cfg.ProxyPath(),x509_user_proxy_new);
    SET_NEW_VAR("CA_CERT_DIR",cfg.CACertificatesDirectory());
  }

  CertEnvLocker::~CertEnvLocker(void) {
    SET_OLD_VAR("CA_CERT_DIR",ca_cert_dir_old,ca_cert_dir_set);
    SET_OLD_VAR("X509_USER_PROXY",x509_user_proxy_old,x509_user_proxy_set);
    SET_OLD_VAR("X509_USER_CERT",x509_user_cert_old,x509_user_cert_set);
    SET_OLD_VAR("X509_USER_KEY",x509_user_key_old,x509_user_key_set);
    if(!x509_user_key_new.empty()) ::unlink(x509_user_key_new.c_str());
    if(!x509_user_cert_new.empty()) ::unlink(x509_user_cert_new.c_str());
    if(!x509_user_proxy_new.empty()) ::unlink(x509_user_proxy_new.c_str());
    EnvLockRelease();
  }

} // namespace Arc
