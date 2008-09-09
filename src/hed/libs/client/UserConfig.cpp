#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <arc/ArcLocation.h>
#include <arc/Logger.h>
#include <arc/StringConv.h>
#include <arc/User.h>
#include <arc/Utils.h>
#include <arc/client/UserConfig.h>

#ifdef HAVE_GLIBMM_GETENV
#include <glibmm/miscutils.h>
#define GetEnv(NAME) Glib::getenv(NAME)
#else
#define GetEnv(NAME) (getenv(NAME)?getenv(NAME):"")
#endif

namespace Arc {

  Logger UserConfig::logger(Logger::getRootLogger(), "UserConfig");

  UserConfig::UserConfig(const std::string& file)
    : conffile(file),
      ok(false) {

    struct stat st;

    if (stat(user.Home().c_str(), &st) != 0){
      logger.msg(ERROR, "Can not access user's home directory: %s (%s)",
		 user.Home(), StrError());
      return;
    }

    if (!S_ISDIR(st.st_mode)) {
      logger.msg(ERROR, "User's home directory is not a directory: %s",
		 user.Home());
      return;
    }

    confdir = user.Home() + "/.arc";

    if (stat(confdir.c_str(), &st) != 0) {
      if (errno == ENOENT) {
	if (mkdir(confdir.c_str(), S_IRWXU) != 0) {
	  logger.msg(ERROR, "Can not create ARC user config directory: "
		     "%s (%s)", confdir, StrError());
	  return;
	}
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
      conffile = confdir + "/client.xml";

    if (stat(conffile.c_str(), &st) != 0) {
      if (errno == ENOENT) {
	NS ns;
	Config(ns).save(conffile.c_str());
	logger.msg(INFO, "Created empty ARC user config file: %s", conffile);
	stat(conffile.c_str(), &st);
      }
      else {
	logger.msg(ERROR, "Can not access ARC user config file: "
		   "%s (%s)", conffile, StrError());
	return;
      }
    }

    if (!S_ISREG(st.st_mode)) {
      logger.msg(ERROR, "ARC user config file is not a regular file: %s",
		 conffile);
      return;
    }

    jobsfile = confdir + "/jobs.xml";

    if (stat(jobsfile.c_str(), &st) != 0) {
      if (errno == ENOENT) {
	NS ns;
	Config(ns).save(jobsfile.c_str());
	logger.msg(INFO, "Created empty ARC user jobs file: %s", jobsfile);
	stat(jobsfile.c_str(), &st);
      }
      else {
	logger.msg(ERROR, "Can not access ARC user jobs file: "
		   "%s (%s)", jobsfile, StrError());
	return;
      }
    }

    if (!S_ISREG(st.st_mode)) {
      logger.msg(ERROR, "ARC user jobs file is not a regular file: %s",
		 jobsfile);
      return;
    }

    cfg.ReadFromFile(conffile);
    syscfg.ReadFromFile(ArcLocation::Get() + "/etc/arcclient.xml");
    if(!syscfg)
      syscfg.ReadFromFile("/etc/arcclient.xml");
    if(!syscfg)
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

  const std::string& UserConfig::JobsFile() const {
    return jobsfile;
  }

  bool UserConfig::ApplySecurity(XMLNode &ccfg) const {

    std::string path;
    struct stat st;

    if (!(path = GetEnv("X509_USER_PROXY")).empty()) {
      if (stat(path.c_str(), &st) != 0) {
	logger.msg(ERROR, "Can not access proxy file: %s(%s)",
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
	logger.msg(ERROR, "Can not access certificate file: %s(%s)",
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
	logger.msg(ERROR, "Can not access key file: %s(%s)",
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

    else if (!(path = (std::string) cfg["ProxyPath"]).empty()) {
      if (stat(path.c_str(), &st) != 0) {
	logger.msg(ERROR, "Can not access proxy file: %s(%s)",
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

    else if (!(path = (std::string) cfg["CertificatePath"]).empty()) {
      if (stat(path.c_str(), &st) != 0) {
	logger.msg(ERROR, "Can not access certificate file: %s(%s)",
		   path, StrError());
	return false;
      }
      if (!S_ISREG(st.st_mode)) {
	logger.msg(ERROR, "Certificate is not a file: %s", path);
	return false;
      }
      logger.msg(INFO, "Using certificate file: %s", path);
      ccfg.NewChild("CertificatePath") = path;

      path = (std::string) cfg["KeyPath"];
      if (path.empty()) {
	logger.msg(ERROR, "CertificatePath defined, but not KeyPath");
	return false;
      }
      if (stat(path.c_str(), &st) != 0) {
	logger.msg(ERROR, "Can not access key file: %s(%s)",
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

    else if (stat((path = "/tmp/x509up_u" +
		   tostring(user.get_uid())).c_str(), &st) == 0) {
      if (!S_ISREG(st.st_mode)) {
	logger.msg(ERROR, "Proxy is not a file: %s", path);
	return false;
      }
      logger.msg(INFO, "Using proxy file: %s", path);
      ccfg.NewChild("ProxyPath") = path;
    }

    else {
      logger.msg (WARNING, "Default proxy file does not exist: %s "
		  "trying default certificate and key", path);
      if (user.get_uid() == 0)
	path = "/etc/grid-security/hostcert.pem";
      else
	path = user.Home() + "/.globus/usercert.pem";
      if (stat(path.c_str(), &st) != 0) {
	logger.msg(ERROR, "Can not access certificate file: %s(%s)",
		   path, StrError());
	return false;
      }
      if (!S_ISREG(st.st_mode)) {
	logger.msg(ERROR, "Certificate is not a file: %s", path);
	return false;
      }
      logger.msg(INFO, "Using certificate file: %s", path);
      ccfg.NewChild("CertificatePath") = path;

      if (user.get_uid() == 0)
	path = "/etc/grid-security/hostkey.pem";
      else
	path = user.Home() + "/.globus/userkey.pem";
      if (stat(path.c_str(), &st) != 0) {
	logger.msg(ERROR, "Can not access key file: %s(%s)", path, StrError());
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
	logger.msg(ERROR, "Can not access CA certificate directory: %s(%s)",
		   path, StrError());
	return false;
      }
      if (!S_ISDIR(st.st_mode)) {
	logger.msg(ERROR, "CA certificate directory is not a directory: %s",
		   path);
	return false;
      }
      ccfg.NewChild("CACertificatesDir") = path;
    }

    else if (!(path = (std::string) cfg["CACertificatesDir"]).empty()) {
      if (stat(path.c_str(), &st) != 0) {
	logger.msg(ERROR, "Can not access CA certificate directory: %s(%s)",
		   path, StrError());
	return false;
      }
      if (!S_ISDIR(st.st_mode)) {
	logger.msg(ERROR, "CA certificate directory is not a directory: %s",
		   path);
	return false;
      }
      logger.msg(INFO, "Using CA certificate directory: %s", path);
      ccfg.NewChild("CACertificatesDir") = path;
    }

    else if (user.get_uid() != 0 &&
	     stat((path = user.Home() +
		   "/.globus/certificates").c_str(), &st) == 0) {
      if (!S_ISDIR(st.st_mode)) {
	logger.msg(ERROR, "CA certificate directory is not a directory: %s",
		   path);
	return false;
      }
      logger.msg(INFO, "Using CA certificate directory: %s", path);
      ccfg.NewChild("CACertificatesDir") = path;
    }

    else {
      path = "/etc/grid-security/certificates";
      if (stat(path.c_str(), &st) != 0) {
	logger.msg(ERROR, "Can not access CA certificate directory: %s(%s)",
		   path, StrError());
	return false;
      }
      if (!S_ISDIR(st.st_mode)) {
	logger.msg(ERROR, "CA Certifcate directory is not a directory: %s",
		   path);
	return false;
      }
      logger.msg(INFO, "Using CA certificate directory: %s", path);
      ccfg.NewChild("CACertificatesDir") = path;
    }

    return true;
  }

  UserConfig::operator bool() const {
    return ok;
  }

  bool UserConfig::operator!() const {
    return !ok;
  }

  std::list<std::string> UserConfig::ResolveAlias(const std::string&
						  lookup) const {
    std::list<std::string> res;

    //find alias in alias listing
    XMLNodeList reslist = cfg.XPathLookup("//Alias[@name='" + lookup + "']",
					  Arc::NS());
    if (reslist.empty())
      res.push_back(lookup);
    else {
      for (XMLNodeList::iterator it = reslist.begin();
	   it != reslist.end(); it++) {

	for(XMLNode node = (*it)["URL"]; node; ++node)
	  res.push_back((std::string) (node));

	for(XMLNode node = (*it)["Alias"]; node; ++node) {
	  std::list<std::string> moreurls = ResolveAlias(node);
	  res.insert(res.end(), moreurls.begin(), moreurls.end());
	}
      }
    }

    return res;
  }

} // namespace Arc
