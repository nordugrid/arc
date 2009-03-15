// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_ARCLOCATION_H__
#define __ARC_ARCLOCATION_H__

#include <string>
#include <list>

namespace Arc {

  /// Determines ARC installation location
  class ArcLocation {
  public:
    /// Initializes location information
    /** Main source is value of variable ARC_LOCATION,
       otherwise path to executable provided in @path is used.
       If nothing works then warning message is sent to logger
       and initial installation prefix is used. */
    static void Init(std::string path);
    /// Returns ARC installation location
    static const std::string& Get();
    /// Returns ARC plugins directory location
    /** Main source is value of variable ARC_PLUGIN_PATH,
       otherwise path is derived from installation location. */
    static std::list<std::string> GetPlugins();
  private:
    static std::string location;
  };

} // namespace Arc

#endif // __ARC_ARCLOCATION_H__
