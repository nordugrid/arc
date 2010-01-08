// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_FINDERLOADER_H__
#define __ARC_FINDERLOADER_H__

#include <map>
#include <string>

namespace Arc {

  // Index represents (internal) name of plugin, value is name
  // of corresponding loadable module.
  typedef std::map<std::string, std::string> PluginList;

  // TODO: Remove this class by partially moving its functionality
  //  to PluginsFactory and related classes. That should make code
  //  avoid double loading of plugins and remove dependence of
  //  plugin loading library on classes managing user configuration.
  //  That will also help to remove ugly Descriptors() method of
  //  PluginsFactory.
  // This class is fully static.
  class FinderLoader
    /* : Loader */ {
  private:
    /*
    FinderLoader(const Config& cfg)
      : Loader(cfg)  {} */
    FinderLoader() {}
    ~FinderLoader() {}
  public:
    static const PluginList GetPluginList(const std::string& kind);
    static const std::list<std::string> GetLibrariesList(void);
  };

} // namespace Arc

#endif // __ARC_FINDERLOADER_H__
