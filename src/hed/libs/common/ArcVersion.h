// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_ARCVERSION_H__
#define __ARC_ARCVERSION_H__

/// \file ArcVersion.h
/** ARC API version */
#define ARC_VERSION "3.0.0"
/** ARC API version number */
#define ARC_VERSION_NUM 0x030000
/** ARC API major version number */
#define ARC_VERSION_MAJOR 3
/** ARC API minor version number */
#define ARC_VERSION_MINOR 0
/** ARC API patch number */
#define ARC_VERSION_PATCH 0

/// Arc namespace contains all core ARC classes.
namespace Arc {

  // Front page for ARC SDK documentation
  /**
   * \mainpage
   * The ARC Software Development Kit (ARC) is a set of tools that allow
   * manipulation of jobs and data in a Grid environment. The SDK is divided
   * into a set of <a href="modules.html">modules</a> which take care of
   * different aspects of Grid interaction.
   * \version The version of the SDK that this documentation refers to can be
   * found from #ARC_VERSION.
   */

  /// Determines ARC HED libraries version at runtime
  /**
   * ARC also provides pre-processor macros to determine the API version at
   * compile time in \ref ArcVersion.h.
   * \headerfile ArcVersion.h arc/ArcVersion.h
   */
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
