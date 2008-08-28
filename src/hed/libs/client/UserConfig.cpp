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

  UserConfig::UserConfig()
    : ok(false) {

    User user;
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

    ok = true;
  }

  UserConfig::~UserConfig() {}

  const std::string& UserConfig::ConfFile() {
    return conffile;
  }

  const std::string& UserConfig::JobsFile() {
    return jobsfile;
  }

  const XMLNode UserConfig::ConfTree() {

    if (!ok)
      return cfg;

    if (!cfg) {
      cfg.ReadFromFile(conffile);
      syscfg.ReadFromFile(ArcLocation::Get() + "/etc/arcclient.xml");
      if(!syscfg)
	syscfg.ReadFromFile("/etc/arcclient.xml");
      if(!syscfg)
	logger.msg(ERROR, "Could not find system client configuration");

      // Merge system and user configuration
      XMLNode child;
      for (int i = 0; (child = syscfg.Child(i)); i++)
	if (!cfg[child.Name()])
	  for (XMLNode n = child; n; ++n)
	    cfg.NewChild(n);

      User user;
      std::string path;

      // Proxy location
      path = GetEnv("X509_USER_PROXY");
      if (!path.empty())
	if (!cfg["ProxyPath"])
	  cfg.NewChild("ProxyPath") = path;
	else
	  cfg["ProxyPath"] = path;
      else if (!cfg["ProxyPath"])
	cfg.NewChild("ProxyPath") = "/tmp/x509up_u" + tostring(user.get_uid());

      // Certificate location
      path = GetEnv("X509_USER_CERT");
      if (!path.empty())
	if (!cfg["CertificatePath"])
	  cfg.NewChild("CertificatePath") = path;
	else
	  cfg["CertificatePath"] = path;
      else if (!cfg["CertificatePath"])
	if (user.get_uid() == 0)
	  cfg.NewChild("CertificatePath") = "/etc/grid-security/hostcert.pem";
	else
	  cfg.NewChild("CertificatePath") =
	    user.Home() + "/.globus/usercert.pem";

      // Private key location
      path = GetEnv("X509_USER_KEY");
      if (!path.empty())
	if (!cfg["KeyPath"])
	  cfg.NewChild("KeyPath") = path;
	else
	  cfg["KeyPath"] = path;
      else if (!cfg["KeyPath"])
	if (user.get_uid() == 0)
	  cfg.NewChild("KeyPath") = "/etc/grid-security/hostkey.pem";
	else
	  cfg.NewChild("KeyPath") =
	    user.Home() + "/.globus/userkey.pem";

      // CA certificate directory location
      path = GetEnv("X509_CERT_DIR");
      if (!path.empty())
	if (!cfg["CACertificatesDir"])
	  cfg.NewChild("CACertificatesDir") = path;
	else
	  cfg["CACertificatesDir"] = path;
      else if (!cfg["CACertificatesDir"])
	cfg.NewChild("CACertificatesDir") = "/etc/grid-security/certificates";
    }

    return cfg;
  }

  UserConfig::operator bool() {
    return ok;
  }

  bool UserConfig::operator!() {
    return !ok;
  }

  std::list<std::string> UserConfig::ResolveAlias(std::string lookup, XMLNode cfg){
    
    std::list<std::string> ToBeReturned;
    
    //find alias in alias listing
    XMLNodeList ResultList = cfg.XPathLookup("//Alias[@name='"+lookup+"']", Arc::NS());
    if(ResultList.empty()){
      ToBeReturned.push_back(lookup);
    } else{

      XMLNode Result = *ResultList.begin();
      
      //read out urls from found alias
      XMLNodeList URLs = Result.XPathLookup("//URL", Arc::NS());
      
      for(XMLNodeList::iterator iter = URLs.begin(); iter != URLs.end(); iter++){
	ToBeReturned.push_back((std::string) (*iter));
      }
      //finally check for other aliases with existing alias
      
      for(XMLNode node = Result["Alias"]; node; ++node){
	std::list<std::string> MoreURLs = ResolveAlias(node, cfg);
	ToBeReturned.insert(ToBeReturned.end(), MoreURLs.begin(), MoreURLs.end());
      }
    }

    return ToBeReturned;

  }

} // namespace Arc
