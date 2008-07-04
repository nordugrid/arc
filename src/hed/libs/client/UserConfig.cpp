#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <arc/ArcConfig.h>
#include <arc/Logger.h>
#include <arc/User.h>
#include <arc/Utils.h>
#include <arc/client/UserConfig.h>

namespace Arc {

  Logger UserConfig::logger(Logger::getRootLogger(), "UserConfig");

  UserConfig::UserConfig()
    : ok(false) {

    User user;
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

    conffile = confdir + "/config.xml";

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


  UserConfig::operator bool() {
    return ok;
  }


  bool UserConfig::operator!() {
    return !ok;
  }
} // namespace Arc
