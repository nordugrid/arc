// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_FINDERLOADER_H__
#define __ARC_FINDERLOADER_H__

#include <map>
#include <string>

#include <arc/loader/Loader.h>

namespace Arc {

  typedef std::map<std::string, std::string> PluginList;

  class FinderLoader
    : Loader {
  private:
    FinderLoader(const Config& cfg)
      : Loader(cfg) {}
    ~FinderLoader() {}
    const PluginList iGetPluginList(const std::string& type);
  public:
    static const PluginList GetPluginList(const std::string& type);
  };

} // namespace Arc

#endif // __ARC_FINDERLOADER_H__
