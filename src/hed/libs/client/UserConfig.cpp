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
#include <arc/client/UserConfig.h>

namespace Arc {

  Logger UserConfig::logger(Logger::getRootLogger(), "UserConfig");
    
  typedef std::vector<std::string> strv_t;

  UserConfig::UserConfig(const std::string& file)
    : conffile(file),
      ok(false) {

    struct stat st;

    if (stat(user.Home().c_str(), &st) != 0) {
      logger.msg(ERROR, "Can not access user's home directory: %s (%s)",
                 user.Home(), StrError());
      return;
    }

    if (!S_ISDIR(st.st_mode)) {
      logger.msg(ERROR, "User's home directory is not a directory: %s",
                 user.Home());
      return;
    }

    strv_t confdirPath (2);
    confdirPath[0] = user.Home();
    confdirPath[1] = ".arc";
    confdir = Glib::build_path(G_DIR_SEPARATOR_S, confdirPath);

    if (stat(confdir.c_str(), &st) != 0) {
      if (errno == ENOENT) {
        #ifndef WIN32
        if (mkdir(confdir.c_str(), S_IRWXU) != 0) {
          logger.msg(ERROR, "Can not create ARC user config directory: %s (%s)",
                     confdir, StrError());
          return;
        }
        #else
        if (mkdir(confdir.c_str()) != 0) { // Create the directory
          logger.msg(ERROR, "Can not create ARC user config directory: %s (%s)",
                     confdir, StrError());
          return;
        }
        #endif

        logger.msg(INFO, "Created ARC user config directory: %s", confdir);
        stat(confdir.c_str(), &st);
      }
      else {
        logger.msg(ERROR, "Can not access ARC user config directory: "
                   "%s (%s)", confdir, StrError());
        return;
      }
    }

    if (!S_ISDIR(st.st_mode)) {
      logger.msg(ERROR, "ARC user config directory is not a directory: %s",
                 confdir);
      return;
    }

    if (conffile.empty())
      conffile = Glib::build_filename(confdir, "client.xml");

    if (stat(conffile.c_str(), &st) != 0) {
      if (errno == ENOENT) {
        NS ns;
        Config(ns).SaveToFile(conffile);
        logger.msg(INFO, "Created empty ARC user config file: %s", conffile);
        stat(conffile.c_str(), &st);
      }
      else {
        logger.msg(ERROR, "Can not access ARC user config file: %s (%s)",
                   conffile, StrError());
        return;
      }
    }

    if (!S_ISREG(st.st_mode)) {
      logger.msg(ERROR, "ARC user config file is not a regular file: %s",
                 conffile);
      return;
    }

    joblistfile  = Glib::build_filename(confdir, "jobs.xml");

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
        return;
      }
    }

    if (!S_ISREG(st.st_mode)) {
      logger.msg(ERROR, "ARC job list file is not a regular file: %s",
                 joblistfile);
      return;
    }

    cfg.ReadFromFile(conffile);

    std::string arcclientconf = G_DIR_SEPARATOR_S +  Glib::build_filename("etc", "arcclient.xml");

    syscfg.ReadFromFile(ArcLocation::Get() + arcclientconf);
    if (!syscfg)
      syscfg.ReadFromFile(arcclientconf);
    if (!syscfg)
      logger.msg(WARNING, "Could not find system client configuration");

    // Merge system and user configuration
    XMLNode child;
    for (int i = 0; (child = syscfg.Child(i)); i++)
      if (!cfg[child.Name()])
        for (XMLNode n = child; n; ++n)
          cfg.NewChild(n);
    ok = true;
  }

  UserConfig::~UserConfig() {}

  const std::string& UserConfig::ConfFile() const {
    return conffile;
  }

  const std::string& UserConfig::JobListFile() const {
    return joblistfile;
  }

  const XMLNode& UserConfig::ConfTree() const {
    return cfg;
  }

  bool UserConfig::ApplySecurity(XMLNode& ccfg) const {

    std::string path;
    struct stat st;

    if (!(path = GetEnv("X509_USER_PROXY")).empty()) {
      if (stat(path.c_str(), &st) != 0) {
        logger.msg(ERROR, "Can not access proxy file: %s (%s)",
                   path, StrError());
        return false;
      }
      if (!S_ISREG(st.st_mode)) {
        logger.msg(ERROR, "Proxy is not a file: %s", path);
        return false;
      }
      logger.msg(INFO, "Using proxy file: %s", path);
      ccfg.NewChild("ProxyPath") = path;
    }
    else if (!(path = GetEnv("X509_USER_CERT")).empty()) {
      if (stat(path.c_str(), &st) != 0) {
        logger.msg(ERROR, "Can not access certificate file: %s (%s)",
                   path, StrError());
        return false;
      }
      if (!S_ISREG(st.st_mode)) {
        logger.msg(ERROR, "Certificate is not a file: %s", path);
        return false;
      }
      logger.msg(INFO, "Using certificate file: %s", path);
      ccfg.NewChild("CertificatePath") = path;

      path = GetEnv("X509_USER_KEY");
      if (path.empty()) {
        logger.msg(ERROR, "X509_USER_CERT defined, but not X509_USER_KEY");
        return false;
      }
      if (stat(path.c_str(), &st) != 0) {
        logger.msg(ERROR, "Can not access key file: %s (%s)",
                   path, StrError());
        return false;
      }
      if (!S_ISREG(st.st_mode)) {
        logger.msg(ERROR, "Key is not a file: %s", path);
        return false;
      }
      logger.msg(INFO, "Using key file: %s", path);
      ccfg.NewChild("KeyPath") = path;
    }
    else if (!(path = (std::string)cfg["ProxyPath"]).empty()) {
      if (stat(path.c_str(), &st) != 0) {
        logger.msg(ERROR, "Can not access proxy file: %s (%s)",
                   path, StrError());
        return false;
      }
      if (!S_ISREG(st.st_mode)) {
        logger.msg(ERROR, "Proxy is not a file: %s", path);
        return false;
      }
      logger.msg(INFO, "Using proxy file: %s", path);
      ccfg.NewChild("ProxyPath") = path;
    }
    else if (!(path = (std::string)cfg["CertificatePath"]).empty()) {
      if (stat(path.c_str(), &st) != 0) {
        logger.msg(ERROR, "Can not access certificate file: %s (%s)",
                   path, StrError());
        return false;
      }
      if (!S_ISREG(st.st_mode)) {
        logger.msg(ERROR, "Certificate is not a file: %s", path);
        return false;
      }
      logger.msg(INFO, "Using certificate file: %s", path);
      ccfg.NewChild("CertificatePath") = path;

      path = (std::string)cfg["KeyPath"];
      if (path.empty()) {
        logger.msg(ERROR, "CertificatePath defined, but not KeyPath");
        return false;
      }
      if (stat(path.c_str(), &st) != 0) {
        logger.msg(ERROR, "Can not access key file: %s (%s)",
                   path, StrError());
        return false;
      }
      if (!S_ISREG(st.st_mode)) {
        logger.msg(ERROR, "Key is not a file: %s", path);
        return false;
      }
      logger.msg(INFO, "Using key file: %s", path);
      ccfg.NewChild("KeyPath") = path;
    }
    else if (stat((path = G_DIR_SEPARATOR_S + Glib::build_filename(std::string("tmp"), std::string("x509up_u") + tostring(user.get_uid()))).c_str(), &st) == 0) {
      if (!S_ISREG(st.st_mode)) {
        logger.msg(ERROR, "Proxy is not a file: %s", path);
        return false;
      }
      logger.msg(INFO, "Using proxy file: %s", path);
      ccfg.NewChild("ProxyPath") = path;
    }
    else {
      logger.msg(WARNING, "Default proxy file does not exist: %s "
                 "trying default certificate and key", path);
      if (user.get_uid() == 0){
        strv_t hostcert(3);
        hostcert[0] = "etc";
        hostcert[1] = "grid-security";
        hostcert[2] = "hostcert.pem";
        path = G_DIR_SEPARATOR_S + Glib::build_filename(hostcert);
      }
      else{
        strv_t usercert (3);
        usercert[0] = user.Home();
        usercert[1] = ".globus";
        usercert[2] = "usercert.pem";
        path = Glib::build_filename(usercert);
      }
      if (stat(path.c_str(), &st) != 0) {
        logger.msg(ERROR, "Can not access certificate file: %s (%s)", path, StrError());
        return false;
      }
      if (!S_ISREG(st.st_mode)) {
        logger.msg(ERROR, "Certificate is not a file: %s", path);
        return false;
      }
      logger.msg(INFO, "Using certificate file: %s", path);
      ccfg.NewChild("CertificatePath") = path;

      if (user.get_uid() == 0){
        strv_t hostkey (3);
        hostkey[0] = "etc";
        hostkey[1] = "grid-security";
        hostkey[2] = "hostkey.pem";
        path = G_DIR_SEPARATOR_S + Glib::build_filename(hostkey);
      }
      else{
        strv_t userkey (3);
        userkey[0] = user.Home();
        userkey[1] = ".globus";
        userkey[2] = "userkey.pem";
        path = Glib::build_filename(userkey);
      }
      if (stat(path.c_str(), &st) != 0) {
        logger.msg(ERROR, "Can not access key file: %s (%s)",
                   path, StrError());
        return false;
      }
      if (!S_ISREG(st.st_mode)) {
        logger.msg(ERROR, "Key is not a file: %s", path);
        return false;
      }
      logger.msg(INFO, "Using key file: %s", path);
      ccfg.NewChild("KeyPath") = path;
    }

    if (!(path = GetEnv("X509_CERT_DIR")).empty()) {
      if (stat(path.c_str(), &st) != 0) {
        logger.msg(ERROR, "Can not access CA certificate directory: %s (%s)",
                   path, StrError());
        return false;
      }
      if (!S_ISDIR(st.st_mode)) {
        logger.msg(ERROR, "CA certificate directory is not a directory: %s", path);
        return false;
      }
      ccfg.NewChild("CACertificatesDir") = path;
    }
    else if (!(path = (std::string)cfg["CACertificatesDir"]).empty()) {
      if (stat(path.c_str(), &st) != 0) {
        logger.msg(ERROR, "Can not access CA certificate directory: %s (%s)",
                   path, StrError());
        return false;
      }
      if (!S_ISDIR(st.st_mode)) {
        logger.msg(ERROR, "CA certificate directory is not a directory: %s", path);
        return false;
      }
      logger.msg(INFO, "Using CA certificate directory: %s", path);
      ccfg.NewChild("CACertificatesDir") = path;
    }
    else if (user.get_uid() != 0 &&
             stat((path = user.Home() + G_DIR_SEPARATOR_S + ".globus" + G_DIR_SEPARATOR_S + "certificates").c_str(), &st) == 0) {
      if (!S_ISDIR(st.st_mode)) {
        logger.msg(ERROR, "CA certificate directory is not a directory: %s", path);
        return false;
      }
      logger.msg(INFO, "Using CA certificate directory: %s", path);
      ccfg.NewChild("CACertificatesDir") = path;
    }
    else {
      strv_t certificatesPath (3);
      certificatesPath[0] = "etc";
      certificatesPath[1] = "grid-security";
      certificatesPath[2] = "certificates";
      path = G_DIR_SEPARATOR_S + Glib::build_filename(certificatesPath);
      if (stat(path.c_str(), &st) != 0) {
        logger.msg(ERROR, "Can not access CA certificate directory: %s (%s)",
                   path, StrError());
        return false;
      }
      if (!S_ISDIR(st.st_mode)) {
        logger.msg(ERROR, "CA certificate directory is not a directory: %s", path);
        return false;
      }
      logger.msg(INFO, "Using CA certificate directory: %s", path);
      ccfg.NewChild("CACertificatesDir") = path;
    }

    return true;
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

    XMLNodeList aliaslist = cfg.XPathLookup("//Alias[@name='" +
                                            alias + "']", NS());

    if (aliaslist.empty()) {
      logger.msg(ERROR, "Alias \"%s\" requested but not defined", alias);
      return false;
    }

    XMLNode& aliasnode = *aliaslist.begin();

    for (XMLNode node = aliasnode["URL"]; node; ++node) {
      std::string flavour = (std::string)node.Attribute("Flavour");
      if (flavour.empty()) {
        logger.msg(ERROR, "URL entry in alias definition \"%s\" has no "
                   "\"Flavour\" attribute", alias);
        return false;
      }
      std::string serviceType = (std::string)node.Attribute("ServiceType");
      if (flavour.empty()) {
        logger.msg(ERROR, "URL entry in alias definition \"%s\" has no "
                   "\"ServiceType\" attribute", alias);
        return false;
      }
      std::string urlstr = (std::string)node;
      if (urlstr.empty()) {
        logger.msg(ERROR, "URL entry in alias definition \"%s\" is empty", alias);
        return false;
      }
      URL url(urlstr);
      if (!url) {
        logger.msg(ERROR, "URL entry in alias definition \"%s\" is not a "
                   "valid URL: %s", alias, urlstr);
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
        return false;
      }
    }

    for (XMLNode node = aliasnode["Alias"]; node; ++node) {
      std::string aliasstr = (std::string)node;
      if (aliasstr.empty()) {
        logger.msg(ERROR, "Alias entry in alias definition \"%s\" is empty", alias);
        return false;
      }
      if (!ResolveAlias(aliasstr, cluster, index))
        return false;
    }
    logger.msg(INFO, "Done resolving alias: %s", alias);
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

  UserConfig::operator bool() const {
    return ok;
  }

  bool UserConfig::operator!() const {
    return !ok;
  }

} // namespace Arc
