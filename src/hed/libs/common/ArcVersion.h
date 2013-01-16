// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_ARCVERSION_H__
#define __ARC_ARCVERSION_H__

/// Arc namespace contains all core ARC classes.
namespace Arc {

  /// Determines ARC HED libraries version
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

  extern const ArcVersion Version;

} // namespace Arc

#endif // __ARC_ARCVERSION_H__
