// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_ARCVERSION_H__
#define __ARC_ARCVERSION_H__

namespace Arc {

  /// Determines ARC HED libraries version
  class ArcVersion {
  public:
    const unsigned int Major;
    const unsigned int Minor;
    const unsigned int Patch;
    ArcVersion(const char* ver);
  };

  extern const ArcVersion Version;

} // namespace Arc

#endif // __ARC_ARCVERSION_H__
