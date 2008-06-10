#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <algorithm>

#include <glibmm/fileutils.h>

#include <arc/message/MCC.h>

namespace Arc {

  Arc::Logger Arc::MCC::logger(Arc::Logger::getRootLogger(), "MCC");

  void MCC::Next(MCCInterface *next, const std::string& label) {
    if (next == NULL)
      next_.erase(label);
    else
      next_[label] = next;
  }

  MCCInterface *MCC::Next(const std::string& label) {
    std::map<std::string, MCCInterface *>::iterator n = next_.find(label);
    if (n == next_.end())
      return NULL;
    return n->second;
  }

  void MCC::Unlink() {
    for (std::map<std::string, MCCInterface *>::iterator n = next_.begin();
	 n != next_.end(); n = next_.begin())
      next_.erase(n);
  }

  void MCC::AddSecHandler(Config *cfg, ArcSec::SecHandler *sechandler,
			  const std::string& label) {
    if (sechandler) {
      sechandlers_[label].push_back(sechandler);
      // need polishing to put the SecHandlerFactory->getinstance here
      XMLNode cn = (*cfg)["SecHandler"];
      Config cfg_(cn);
    }
  }

  bool MCC::ProcessSecHandlers(Arc::Message& message,
			       const std::string& label) {
    // Each MCC/Service can define security handler queues in the configuration
    // file, the queues have labels specified in handlers configuration 'event'
    // attribute.
    // Security handlers in one queue are called sequentially.
    // Each one should be configured carefully, because there can be some
    // relationship between them (e.g. authentication should be put in front
    // of authorization).
    // The SecHandler::Handle() only returns true/false with true meaning that
    // handler processed message successfuly. If SecHandler implements
    // authorization functionality, it returns false if message is disallowed
    // and true otherwise.
    // If any SecHandler in the handler chain produces some information which
    // will be used by some following handler, the information should be
    // stored in the attributes of message (e.g. the Identity extracted from
    // authentication will be used by authorization to make access control
    // decision).
    std::map<std::string, std::list<ArcSec::SecHandler *> >::iterator q =
      sechandlers_.find(label);
    if (q == sechandlers_.end()) {
      logger.msg(Arc::VERBOSE,
		 "No security processing/check requested for '%s'", label);
      return true;
    }
    for (std::list<ArcSec::SecHandler *>::iterator h = q->second.begin();
	 h != q->second.end(); ++h) {
      ArcSec::SecHandler *handler = *h;
      if (!handler)
	continue; // Shouldn't happen. Just a sanity check.
      if (!(handler->Handle(&message))) {
	logger.msg(Arc::INFO, "Security processing/check failed");
	return false;
      }
    }
    logger.msg(Arc::VERBOSE, "Security processing/check passed");
    return true;
  }

  XMLNode MCCConfig::MakeConfig(XMLNode cfg) const {
    XMLNode mm = BaseConfig::MakeConfig(cfg);
    std::list<std::string> mccs;
    for (std::list<std::string>::const_iterator path = plugin_paths.begin();
	 path != plugin_paths.end(); path++) {
      try {
	Glib::Dir dir(*path);
	for (Glib::DirIterator file = dir.begin(); file != dir.end(); file++) {
	  if ((*file).substr(0, 6) == "libmcc") {
	    std::string name = (*file).substr(6, (*file).find('.') - 6);
	    if (std::find(mccs.begin(), mccs.end(), name) == mccs.end()) {
	      mccs.push_back(name);
	      cfg.NewChild("Plugins").NewChild("Name") = "mcc" + name;
	    }
	  }
          //Since the security handler could also be used by mcc like tls and soap,
          //putting the libarcpdc here. Here we suppose all of the sec handlers are
          //put in libarcpdp
          if ((*file).substr(0, 9) == "libarcpdc") {
            cfg.NewChild("Plugins").NewChild("Name") = "arcpdc";
          }
        }
      }
      catch (Glib::FileError) {}
    }
    return mm;
  }

} // namespace Arc
