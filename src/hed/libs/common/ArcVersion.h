// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_ARCVERSION_H__
#define __ARC_ARCVERSION_H__

#define ARC_VERSION "3.0.0"
#define ARC_VERSION_NUM 0x030000
#define ARC_VERSION_MAJOR 3
#define ARC_VERSION_MINOR 0
#define ARC_VERSION_PATCH 0

/// Arc namespace contains all core ARC classes.
namespace Arc {

  /// Determines ARC HED libraries version at runtime
  /** \headerfile ArcVersion.h arc/ArcVersion.h */
  class ArcVersion {
  public:
    /// Major version number
    const unsigned int Major;
    /// Minor version number
    const unsigned int Minor;
    /// Patch version number
    const unsigned int Patch;
    /// Parses ver and fills major, minor and patch version values
    ArcVersion(const char* ver);
  };

  /// Use this object to obtain current ARC HED version 
  /// at runtime.
  extern const ArcVersion Version;

} // namespace Arc

#endif // __ARC_ARCVERSION_H__
