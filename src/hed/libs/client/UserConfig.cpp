// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <glibmm/miscutils.h>

#include <arc/ArcLocation.h>
#include <arc/Logger.h>
#include <arc/StringConv.h>
#include <arc/URL.h>
#include <arc/User.h>
#include <arc/Utils.h>
#include <arc/client/JobSupervisor.h>
#include <arc/client/JobController.h>
#include <arc/client/UserConfig.h>
#include <arc/credential/Credential.h>

namespace Arc {

  Logger UserConfig::logger(Logger::getRootLogger(), "UserConfig");

  typedef std::vector<std::string> strv_t;

  std::list<std::string> UserConfig::resolvedAlias;
  
  const std::string UserConfig::DEFAULT_BROKER = "RandomBroker";

  UserConfig::UserConfig(const std::string& file, bool initializeCredentials)
    : conffile(file), userSpecifiedJobList(false), ok(false) {
    if (!loadUserConfiguration(file))
      return;

    if (initializeCredentials) {
      InitializeCredentials();
      if (!CredentialsFound()) return;
    }

    ok = true;

    setDefaults();
  }

  UserConfig::UserConfig(const std::string& file, const std::string& jfile, bool initializeCredentials)
    : conffile(file), joblistfile(jfile), userSpecifiedJobList(!jfile.empty()), ok(false)
  {
    if (!loadUserConfiguration(file))
      return;

    struct stat st;

    strv_t confdirPath(2);
    confdirPath[0] = user.Home();
    confdirPath[1] = ".arc";
    const std::string confdir = Glib::build_path(G_DIR_SEPARATOR_S, confdirPath);

    // First check if job list file was given as an argument, then look for it
    // in the user configuration, and last set job list file to default.
    if (joblistfile.empty() &&
        (joblistfile = (std::string)cfg["JobListFile"]).empty())
      joblistfile = Glib::build_filename(confdir, "jobs.xml");

    // Check if joblistfile exist. If not try to create a empty version, and if
    // this fails report error an exit.
    if (stat(joblistfile.c_str(), &st) != 0) {
      if (errno == ENOENT) {
        NS ns;
        Config(ns).SaveToFile(joblistfile);
        logger.msg(INFO, "Created empty ARC job list file: %s", joblistfile);
        stat(joblistfile.c_str(), &st);
      }
      else {
        logger.msg(ERROR, "Can not access ARC job list file: %s (%s)",
                   joblistfile, StrError());
        std::cerr << IString("Can not access ARC job list file: %s (%s)",
                             joblistfile, StrError()) << std::endl;
        return;
      }
    }
    else if (!S_ISREG(st.st_mode)) {
      logger.msg(ERROR, "ARC job list file is not a regular file: %s",
                 joblistfile);
        std::cerr << IString("ARC job list file is not a regular file: %s",
                             joblistfile) << std::endl;
        return;
    }


    if (initializeCredentials) {
      InitializeCredentials();
      if (!CredentialsFound()) return;
    }

    ok = true;

    setDefaults();
  }

  void UserConfig::ApplyToConfig(XMLNode& ccfg) const {
    if (!proxyPath.empty())
      ccfg.NewChild("ProxyPath") = proxyPath;
    else {
      ccfg.NewChild("CertificatePath") = certificatePath;
      ccfg.NewChild("KeyPath") = keyPath;
    }

    ccfg.NewChild("CACertificatesDir") = caCertificatesDir;

    ccfg.NewChild("TimeOut") = (std::string)cfg["TimeOut"];

    if (cfg["Broker"]["Name"]) {
      ccfg.NewChild("Broker").NewChild("Name") = (std::string)cfg["Broker"]["Name"];
      if (cfg["Broker"]["Arguments"])
        ccfg["Broker"].NewChild("Arguments") = (std::string)cfg["Broker"]["Arguments"];
    }
  }

  void UserConfig::ApplyToConfig(BaseConfig& ccfg) const {
    if (!proxyPath.empty())
      ccfg.AddProxy(proxyPath);
    else {
      ccfg.AddCertificate(certificatePath);
      ccfg.AddPrivateKey(keyPath);
    }

    ccfg.AddCADir(caCertificatesDir);
  }

  void UserConfig::SetTimeOut(unsigned int timeOut) {
    if (!cfg["TimeOut"])
      cfg.NewChild("TimeOut");

    cfg["TimeOut"] = tostring(timeOut);
  }

  void UserConfig::SetBroker(const std::string& broker)
  {
    if (!cfg["Broker"])
      cfg.NewChild("Broker");
    if (!cfg["Broker"]["Name"])
      cfg["Broker"].NewChild("Name");

    const std::string::size_type pos = broker.find(":");

    cfg["Broker"]["Name"] = broker.substr(0, pos);
    
    if (pos != std::string::npos && pos != broker.size()-1) {
      if (!cfg["Broker"]["Arguments"])
        cfg["Broker"].NewChild("Arguments");
      cfg["Broker"]["Arguments"] = broker.substr(pos+1);
    }
    else if (cfg["Broker"]["Arguments"]) {
      cfg["Broker"]["Arguments"] = "";
    }
  }

  bool UserConfig::DefaultServices(URLListMap& cluster,
                                   URLListMap& index) const {

    logger.msg(INFO, "Finding default services");

    XMLNode defservice = cfg["DefaultServices"];

    for (XMLNode node = defservice["URL"]; node; ++node) {
      std::string flavour = (std::string)node.Attribute("Flavour");
      if (flavour.empty()) {
        logger.msg(ERROR, "URL entry in default services has no "
                   "\"Flavour\" attribute");
        return false;
      }
      std::string serviceType = (std::string)node.Attribute("ServiceType");
      if (flavour.empty()) {
        logger.msg(ERROR, "URL entry in default services has no "
                   "\"ServiceType\" attribute");
        return false;
      }
      std::string urlstr = (std::string)node;
      if (urlstr.empty()) {
        logger.msg(ERROR, "URL entry in default services is empty");
        return false;
      }
      URL url(urlstr);
      if (!url) {
        logger.msg(ERROR, "URL entry in default services is not a "
                   "valid URL: %s", urlstr);
        return false;
      }
      if (serviceType == "computing") {
        logger.msg(INFO, "Default services contain a computing service of "
                   "flavour %s: %s", flavour, url.str());
        cluster[flavour].push_back(url);
      }
      else if (serviceType == "index") {
        logger.msg(INFO, "Default services contain an index service of "
                   "flavour %s: %s", flavour, url.str());
        index[flavour].push_back(url);
      }
      else {
        logger.msg(ERROR, "URL entry in default services contains "
                   "unknown ServiceType: %s", serviceType);
        return false;
      }
    }

    for (XMLNode node = defservice["Alias"]; node; ++node) {
      std::string aliasstr = (std::string)node;
      if (aliasstr.empty()) {
        logger.msg(ERROR, "Alias entry in default services is empty");
        return false;
      }
      if (!ResolveAlias(aliasstr, cluster, index))
        return false;
    }

    logger.msg(INFO, "Done finding default services");

    return true;
  }

  bool UserConfig::ResolveAlias(const std::string alias,
                                URLListMap& cluster,
                                URLListMap& index) const {

    logger.msg(INFO, "Resolving alias: %s", alias);

    for (std::list<std::string>::iterator it = resolvedAlias.begin();
         it != resolvedAlias.end(); it++) {
      if (*it == alias) {
        std::cerr << "Cannot resolve alias \"" << *resolvedAlias.begin() << "\". Loop detected." << std::endl;
        for (std::list<std::string>::iterator itloop = resolvedAlias.begin();
             itloop != resolvedAlias.end(); itloop++) {
          std::cerr << *itloop << " -> ";
        }
        std::cerr << alias << std::endl;
        
        resolvedAlias.clear();
        return false;
      }
    }

    XMLNodeList aliaslist = cfg.XPathLookup("//AliasList/Alias[@name='" +
                                            alias + "']", NS());

    if (aliaslist.empty()) {
      logger.msg(ERROR, "Alias \"%s\" requested but not defined", alias);
      resolvedAlias.clear();
      return false;
    }

    XMLNode& aliasnode = *aliaslist.begin();

    for (XMLNode node = aliasnode["URL"]; node; ++node) {
      std::string flavour = (std::string)node.Attribute("Flavour");
      if (flavour.empty()) {
        logger.msg(ERROR, "URL entry in alias definition \"%s\" has no "
                   "\"Flavour\" attribute", alias);
        resolvedAlias.clear();
        return false;
      }
      std::string serviceType = (std::string)node.Attribute("ServiceType");
      if (flavour.empty()) {
        logger.msg(ERROR, "URL entry in alias definition \"%s\" has no "
                   "\"ServiceType\" attribute", alias);
        resolvedAlias.clear();
        return false;
      }
      std::string urlstr = (std::string)node;
      if (urlstr.empty()) {
        logger.msg(ERROR, "URL entry in alias definition \"%s\" is empty", alias);
        resolvedAlias.clear();
        return false;
      }
      URL url(urlstr);
      if (!url) {
        logger.msg(ERROR, "URL entry in alias definition \"%s\" is not a "
                   "valid URL: %s", alias, urlstr);
        resolvedAlias.clear();
        return false;
      }
      if (serviceType == "computing") {
        logger.msg(INFO, "Alias %s contains a computing service of "
                   "flavour %s: %s", alias, flavour, url.str());
        cluster[flavour].push_back(url);
      }
      else if (serviceType == "index") {
        logger.msg(INFO, "Alias %s contains an index service of "
                   "flavour %s: %s", alias, flavour, url.str());
        index[flavour].push_back(url);
      }
      else {
        logger.msg(ERROR, "URL entry in alias definition \"%s\" contains "
                   "unknown ServiceType: %s", alias, serviceType);
        resolvedAlias.clear();
        return false;
      }
    }

    resolvedAlias.push_back(alias);

    for (XMLNode node = aliasnode["AliasRef"]; node; ++node) {
      std::string aliasstr = (std::string)node;
      if (aliasstr.empty()) {
        logger.msg(ERROR, "Alias entry in alias definition \"%s\" is empty", alias);
        resolvedAlias.clear();
        return false;
      }
      if (!ResolveAlias(aliasstr, cluster, index))
        return false;
    }
    logger.msg(INFO, "Done resolving alias: %s", alias);
    resolvedAlias.clear();
    return true;
  }

  bool UserConfig::ResolveAlias(const std::list<std::string>& clusters,
                                const std::list<std::string>& indices,
                                URLListMap& clusterselect,
                                URLListMap& clusterreject,
                                URLListMap& indexselect,
                                URLListMap& indexreject) const {

    for (std::list<std::string>::const_iterator it = clusters.begin();
         it != clusters.end(); it++) {
      bool select = true;
      std::string cluster = *it;
      if (cluster[0] == '-')
        select = false;
      if ((cluster[0] == '-') || (cluster[0] == '+'))
        cluster = cluster.substr(1);

      std::string::size_type colon = cluster.find(':');
      if (colon == std::string::npos) {
        if (select) {
          if (!ResolveAlias(cluster, clusterselect, indexselect))
            return false;
        }
        else
          if (!ResolveAlias(cluster, clusterreject, indexreject))
            return false;
      }
      else {
        std::string flavour = cluster.substr(0, colon);
        std::string urlstr = cluster.substr(colon + 1);
        URL url(urlstr);
        if (!url) {
          logger.msg(ERROR, "%s is not a valid URL", urlstr);
          return false;
        }
        if (select)
          clusterselect[flavour].push_back(url);
        else
          clusterreject[flavour].push_back(url);
      }
    }

    for (std::list<std::string>::const_iterator it = indices.begin();
         it != indices.end(); it++) {
      bool select = true;
      std::string index = *it;
      if (index[0] == '-')
        select = false;
      if ((index[0] == '-') || (index[0] == '+'))
        index = index.substr(1);

      std::string::size_type colon = index.find(':');
      if (colon == std::string::npos) {
        if (select) {
          if (!ResolveAlias(index, clusterselect, indexselect))
            return false;
        }
        else
          if (!ResolveAlias(index, clusterreject, indexreject))
            return false;
      }
      else {
        std::string flavour = index.substr(0, colon);
        std::string urlstr = index.substr(colon + 1);
        URL url(urlstr);
        if (!url) {
          logger.msg(ERROR, "%s is not a valid URL", urlstr);
          return false;
        }
        if (select)
          indexselect[flavour].push_back(url);
        else
          indexreject[flavour].push_back(url);
      }
    }
    return true;
  }

  bool UserConfig::ResolveAlias(const std::list<std::string>& clusters,
                                URLListMap& clusterselect,
                                URLListMap& clusterreject) const {

    std::list<std::string> indices;
    URLListMap indexselect;
    URLListMap indexreject;
    return ResolveAlias(clusters, indices, clusterselect,
                        clusterreject, indexselect, indexreject);
  }

  bool UserConfig::CheckProxy() const {
    if (!proxyPath.empty()) {
      Credential holder(proxyPath, "",
                        caCertificatesDir, "");
      if (holder.GetEndTime() >= Time())
        logger.msg(INFO, "Valid proxy found");
      else {
        logger.msg(ERROR, "Proxy expired");
        return false;
      }
    }
    else {
      Credential holder(certificatePath,
                        keyPath,
                        caCertificatesDir, "");
      if (holder.GetEndTime() >= Time())
        logger.msg(INFO, "Valid certificate and key found");
      else {
        logger.msg(ERROR, "Certificate and key expired");
        return false;
      }
    }

    return true;
  }

  void UserConfig::InitializeCredentials() {
    struct stat st;
    // Look for credentials.
    if (!(proxyPath = GetEnv("X509_USER_PROXY")).empty()) {
      if (stat(proxyPath.c_str(), &st) != 0) {
        logger.msg(ERROR, "Can not access proxy file: %s (%s)",
                   proxyPath, StrError());
        proxyPath.clear();
      }
      else if (!S_ISREG(st.st_mode)) {
        logger.msg(ERROR, "Proxy is not a file: %s", proxyPath);
        proxyPath.clear();
      }
    }
    else if (!(certificatePath = GetEnv("X509_USER_CERT")).empty() &&
             !(keyPath         = GetEnv("X509_USER_KEY" )).empty()) {
      if (stat(certificatePath.c_str(), &st) != 0) {
        logger.msg(ERROR, "Can not access certificate file: %s (%s)",
                   certificatePath, StrError());
        certificatePath.clear();
      }
      else if (!S_ISREG(st.st_mode)) {
        logger.msg(ERROR, "Certificate is not a file: %s", certificatePath);
        certificatePath.clear();
      }

      if (stat(keyPath.c_str(), &st) != 0) {
        logger.msg(ERROR, "Can not access key file: %s (%s)",
                   keyPath, StrError());
        keyPath.clear();
      }
      else if (!S_ISREG(st.st_mode)) {
        logger.msg(ERROR, "Key is not a file: %s", keyPath);
        keyPath.clear();
      }
    }
    else if (!(proxyPath = (std::string)cfg["ProxyPath"]).empty()) {
      if (stat(proxyPath.c_str(), &st) != 0) {
        logger.msg(ERROR, "Can not access proxy file: %s (%s)",
                   proxyPath, StrError());
        proxyPath.clear();
      }
      else if (!S_ISREG(st.st_mode)) {
        logger.msg(ERROR, "Proxy is not a file: %s", proxyPath);
        proxyPath.clear();
      }
    }
    else if (!(certificatePath = (std::string)cfg["CertificatePath"]).empty() &&
             !(keyPath         = (std::string)cfg["KeyPath"]        ).empty()) {
      if (stat(certificatePath.c_str(), &st) != 0) {
        logger.msg(ERROR, "Can not access certificate file: %s (%s)",
                   certificatePath, StrError());
        certificatePath.clear();
      }
      else if (!S_ISREG(st.st_mode)) {
        logger.msg(ERROR, "Certificate is not a file: %s", certificatePath);
        certificatePath.clear();
      }

      if (stat(keyPath.c_str(), &st) != 0) {
        logger.msg(ERROR, "Can not access key file: %s (%s)",
                   keyPath, StrError());
        keyPath.clear();
      }
      else if (!S_ISREG(st.st_mode)) {
        logger.msg(ERROR, "Key is not a file: %s", keyPath);
        keyPath.clear();
      }
    }
    else if (stat((proxyPath = G_DIR_SEPARATOR_S + Glib::build_filename(std::string("tmp"), std::string("x509up_u") + tostring(user.get_uid()))).c_str(), &st) == 0) {
      if (!S_ISREG(st.st_mode)) {
        logger.msg(ERROR, "Proxy is not a file: %s", proxyPath);
        proxyPath.clear();
      }
    }
    else {
      logger.msg(WARNING, "Default proxy file does not exist: %s "
                 "trying default certificate and key", proxyPath);
      if (user.get_uid() == 0) {
        strv_t hostcert(3);
        hostcert[0] = "etc";
        hostcert[1] = "grid-security";
        hostcert[2] = "hostcert.pem";
        certificatePath = G_DIR_SEPARATOR_S + Glib::build_filename(hostcert);
      }
      else {
        strv_t usercert(3);
        usercert[0] = user.Home();
        usercert[1] = ".globus";
        usercert[2] = "usercert.pem";
        certificatePath = Glib::build_filename(usercert);
      }

      if (stat(certificatePath.c_str(), &st) != 0) {
        logger.msg(ERROR, "Can not access certificate file: %s (%s)", certificatePath, StrError());
        certificatePath.clear();
      }
      else if (!S_ISREG(st.st_mode)) {
        logger.msg(ERROR, "Certificate is not a file: %s", certificatePath);
        certificatePath.clear();
      }
            
      if (user.get_uid() == 0) {
        strv_t hostkey(3);
        hostkey[0] = "etc";
        hostkey[1] = "grid-security";
        hostkey[2] = "hostkey.pem";
        keyPath = G_DIR_SEPARATOR_S + Glib::build_filename(hostkey);
      }
      else {
        strv_t userkey(3);
        userkey[0] = user.Home();
        userkey[1] = ".globus";
        userkey[2] = "userkey.pem";
        keyPath = Glib::build_filename(userkey);
      }

      if (stat(keyPath.c_str(), &st) != 0) {
        logger.msg(ERROR, "Can not access key file: %s (%s)",
                   keyPath, StrError());
        keyPath.clear();
      }
      else if (!S_ISREG(st.st_mode)) {
        logger.msg(ERROR, "Key is not a file: %s", keyPath);
        keyPath.clear();
      }
    }

    if (!(caCertificatesDir = GetEnv("X509_CERT_DIR")).empty()) {
      if (stat(caCertificatesDir.c_str(), &st) != 0) {
        logger.msg(ERROR, "Can not access CA certificate directory: %s (%s)",
                   caCertificatesDir, StrError());
        caCertificatesDir.clear();
      }
      else if (!S_ISDIR(st.st_mode)) {
        logger.msg(ERROR, "CA certificate directory is not a directory: %s", caCertificatesDir);
        caCertificatesDir.clear();
      }
    }
    else if (!(caCertificatesDir = (std::string)cfg["CACertificatesDir"]).empty()) {
      if (stat(caCertificatesDir.c_str(), &st) != 0) {
        logger.msg(ERROR, "Can not access CA certificate directory: %s (%s)",
                   caCertificatesDir, StrError());
        caCertificatesDir.clear();
      }
      else if (!S_ISDIR(st.st_mode)) {
        logger.msg(ERROR, "CA certificate directory is not a directory: %s", caCertificatesDir);
        caCertificatesDir.clear();
      }
    }
    else if (user.get_uid() != 0 &&
             stat((caCertificatesDir = user.Home() + G_DIR_SEPARATOR_S + ".globus" + G_DIR_SEPARATOR_S + "certificates").c_str(), &st) == 0) {
      if (!S_ISDIR(st.st_mode)) {
        logger.msg(ERROR, "CA certificate directory is not a directory: %s", caCertificatesDir);
        caCertificatesDir.clear();
      }
    }
    else {
      strv_t caCertificatesPath(3);
      caCertificatesPath[0] = "etc";
      caCertificatesPath[1] = "grid-security";
      caCertificatesPath[2] = "certificates";
      caCertificatesDir = G_DIR_SEPARATOR_S + Glib::build_filename(caCertificatesPath);
      if (stat(caCertificatesDir.c_str(), &st) != 0) {
        logger.msg(ERROR, "Can not access CA certificate directory: %s (%s)",
                   caCertificatesDir, StrError());
        caCertificatesDir.clear();
      }
      else if (!S_ISDIR(st.st_mode)) {
        logger.msg(ERROR, "CA certificate directory is not a directory: %s", caCertificatesDir);
        caCertificatesDir.clear();
      }
    }


    if (!proxyPath.empty())
      logger.msg(INFO, "Using proxy file: %s", proxyPath);
    else if (!certificatePath.empty() && !keyPath.empty()) {
      logger.msg(INFO, "Using certificate file: %s", certificatePath);
      logger.msg(INFO, "Using key file: %s", keyPath);

    if (!caCertificatesDir.empty())
      logger.msg(INFO, "Using CA certificate directory: %s", caCertificatesDir);
    }
  }

  bool UserConfig::loadUserConfiguration(const std::string& file)
  {
    struct stat st;

    strv_t confdirPath(2);
    confdirPath[0] = user.Home();
    confdirPath[1] = ".arc";
    const std::string confdir = Glib::build_path(G_DIR_SEPARATOR_S, confdirPath);

    // Check if user configuration file exist. If file name was given as
    // argument to the constructor and file does not exist report error and exit.
    // If the constructor received an empty file name look for the default
    // configuration file, if it does not exist continue without loading any
    // user configuration file.
    if (conffile.empty())
      conffile = Glib::build_filename(confdir, "client.xml");

    if (stat(conffile.c_str(), &st) != 0) {
      if (conffile == file) {
        logger.msg(ERROR, "Cannot access ARC user config file: %s (%s)",
                   conffile, StrError());
        return false;
      }
      else conffile.clear();
    }
    else if (!S_ISREG(st.st_mode)) {
      if (conffile == file) {
        logger.msg(ERROR, "ARC user config file is not a regular file: %s",
                   conffile);
        return false;
      }
      else conffile.clear();
    }

    // First try to load system client configuration.
    const std::string arcclientconf = G_DIR_SEPARATOR_S + Glib::build_filename("etc", "arcclient.xml");

    if (!cfg.ReadFromFile(ArcLocation::Get() + arcclientconf) &&
        !cfg.ReadFromFile(arcclientconf))
      logger.msg(WARNING, "Could not load system client configuration");


    if (!conffile.empty()) {
      Config ucfg;
      if (!ucfg.ReadFromFile(conffile)) {
        logger.msg(WARNING, "Could not load user client configuration");
      }
      else {
        // Merge system and user configuration
        XMLNode child;
        for (int i = 0; (child = ucfg.Child(i)); i++) {
          if (child.Name() != "AliasList") {
            if (cfg[child.Name()])
              cfg[child.Name()].Replace(child);
            else 
              cfg.NewChild(child);
          }
          else {
            if (!cfg["AliasList"])
              cfg.NewChild(child);
            else 
              // Look for duplicates. If duplicates exist, keep those defined in
              // the user configuration file.
              for (XMLNode alias = child["Alias"]; alias; ++alias) { // Loop over Alias nodes in user configuration file.
                XMLNodeList aliasList = cfg.XPathLookup("//AliasList/Alias[@name='" + (std::string)alias.Attribute("name") + "']", NS());
                if (!aliasList.empty()) {
                  // Remove duplicates.
                  for (XMLNodeList::iterator node = aliasList.begin(); node != aliasList.end(); node++) {
                    node->Destroy();
                  }
                }
                cfg["AliasList"].NewChild(alias);
              }
          }
        }
      }
    }
    
    return true;
  }

  void UserConfig::setDefaults()
  {
    if (!cfg["TimeOut"])
      cfg.NewChild("TimeOut") = DEFAULT_TIMEOUT;
    else if (((std::string)cfg["TimeOut"]).empty() || stringtoi((std::string)cfg["TimeOut"]) <= 0)
      cfg["TimeOut"] = DEFAULT_TIMEOUT;

    if (!cfg["Broker"]["Name"]) {
      if (!cfg["Broker"])
        cfg.NewChild("Broker");
      cfg["Broker"].NewChild("Name") = DEFAULT_BROKER;
      if (cfg["Broker"]["Arguments"])
        cfg["Broker"]["Arguments"] = "";
    }
  }
} // namespace Arc
