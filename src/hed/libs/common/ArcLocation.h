// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_ARCLOCATION_H__
#define __ARC_ARCLOCATION_H__

#include <string>
#include <list>

namespace Arc {

  /// Determines ARC installation location
  /** \ingroup common
   * \headerfile ArcLocation.h arc/ArcLocation.h */
  class ArcLocation {
  public:
    /// Initializes location information
    /** Main source is value of variable ARC_LOCATION,
       otherwise path to executable provided in path is used.
       If nothing works then warning message is sent to logger
       and initial installation prefix is used. */
    static void Init(std::string path);
    /// Returns ARC installation location
    static const std::string& Get();
    /// Returns ARC plugins directory location
    /** Main source is value of variable ARC_PLUGIN_PATH,
       otherwise path is derived from installation location. */
    static std::list<std::string> GetPlugins();
    /// Returns location of ARC system data, e.g. $ARC_LOCATION/share/arc
    /**
     * \since Added in 3.0.0
     **/
    static std::string GetDataDir();
    /// Returns location of ARC system libraries, e.g. $ARC_LOCATION/lib/arc
    /**
     * \since Added in 4.2.0
     **/
    static std::string GetLibDir();
    /// Returns location of ARC system tools, e.g. $ARC_LOCATION/libexec/arc
    /** 
     * \since Added in 3.0.0
     **/
    static std::string GetToolsDir();
  private:
    static std::string& location(void);
  };

} // namespace Arc

#endif // __ARC_ARCLOCATION_H__
