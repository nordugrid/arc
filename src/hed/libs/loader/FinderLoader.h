// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_FINDERLOADER_H__
#define __ARC_FINDERLOADER_H__

#include <map>
#include <string>

namespace Arc {

  // TODO: Remove this class by partially moving its functionality
  //  to PluginsFactory and related classes. That should remove 
  //  dependency of plugin loading library on classes managing user 
  //  configuration.
  // This class is fully static.
  class FinderLoader
    /* : Loader */ {
  private:
    FinderLoader() {}
    ~FinderLoader() {}
  public:
    //static const PluginList GetPluginList(const std::string& kind);
    static const std::list<std::string> GetLibrariesList(void);
  };

} // namespace Arc

#endif // __ARC_FINDERLOADER_H__
