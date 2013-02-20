// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_PROFILE_H__
#define __ARC_PROFILE_H__

#include <string>

#include <arc/ArcConfig.h>
#include <arc/IniConfig.h>
#include <arc/Logger.h>
#include <arc/XMLNode.h>

namespace Arc {

  /// Class used to convert human-friendly ini-style configuration to XML.
  /** \ingroup common
   *  \headerfile Profile.h arc/Profile.h */
  class Profile
    : public XMLNode {
  public:
    /// Create a new profile with the given profile file
    Profile(const std::string& filename);
    ~Profile();
    /// Evaluate the given ini-style configuration against the current profile.
    void Evaluate(Config &cfg, IniConfig ini);
  };

} // namespace Arc

#endif /* __ARC_PROFILE_H__ */
