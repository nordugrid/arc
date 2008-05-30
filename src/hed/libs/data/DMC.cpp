#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <algorithm>

#include <glibmm/fileutils.h>

#include <arc/Logger.h>
#include <arc/data/DMC.h>
#include <arc/loader/Loader.h>

namespace Arc {

  Logger DMC::logger(Logger::rootLogger, "DMC");

  std::list<DMC *> DMC::dmcs;

  Glib::Mutex DMC::mutex;

  DataPoint *DMC::GetDataPoint(const URL& url) {
    DataPoint *dp = NULL;
    Glib::Mutex::Lock lock(mutex);
    for (std::list<DMC *>::iterator i = dmcs.begin();
	 !dp && (i != dmcs.end()); i++)
      dp = (*i)->iGetDataPoint(url);
    return dp;
  }

  void DMC::Register(DMC *dmc) {
    Glib::Mutex::Lock lock(mutex);
    dmcs.push_back(dmc);
  }

  void DMC::Unregister(DMC *dmc) {
    Glib::Mutex::Lock lock(mutex);
    dmcs.remove(dmc);
  }

  XMLNode DMCConfig::MakeConfig(XMLNode cfg) const {
    XMLNode mm = BaseConfig::MakeConfig(cfg);
    std::list<std::string> dmcs;
    for (std::list<std::string>::const_iterator path = plugin_paths.begin();
	 path != plugin_paths.end(); path++) {
      try {
	Glib::Dir dir(*path);
	for (Glib::DirIterator file = dir.begin(); file != dir.end(); file++)
	  if ((*file).substr(0, 6) == "libdmc") {
	    std::string name = (*file).substr(6, (*file).find('.') - 6);
	    if (std::find(dmcs.begin(), dmcs.end(), name) == dmcs.end()) {
	      dmcs.push_back(name);
	      cfg.NewChild("Plugins").NewChild("Name") = "dmc" + name;
	      XMLNode dm = cfg.NewChild("DataManager");
	      dm.NewAttribute("name") = name;
	      dm.NewAttribute("id") = name;
	    }
	  }
      }
      catch (Glib::FileError) {}
    }
    return mm;
  }

  static class DMCLoader {

  public:
    DMCLoader() {
      DMCConfig dmcconf;
      NS ns;
      Config cfg(ns);
      dmcconf.MakeConfig(cfg);
      loader = new Loader(&cfg);
    }

    ~DMCLoader() {}

  private:
    Loader *loader;

  } dmcloader;

} // namespace Arc
