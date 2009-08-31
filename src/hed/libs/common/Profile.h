// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_PROFILE_H__
#define __ARC_PROFILE_H__

#include <string>

#include <arc/ArcConfig.h>
#include <arc/IniConfig.h>
#include <arc/XMLNode.h>

namespace Arc {

  class Profile
    : public XMLNode {
  public:
    Profile(const std::string& filename);
    ~Profile();
    Config Evaluate(const IniConfig& ini);
  };

} // namespace Arc

#endif /* __ARC_PROFILE_H__ */
