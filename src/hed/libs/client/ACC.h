#ifndef __ARC_ACC_H__
#define __ARC_ACC_H__

#include <arc/ArcConfig.h>

namespace Arc {

  class ACC {
  protected:
    ACC() {}
  public:
    virtual ~ACC() {}
  };

  class ACCConfig
    : public BaseConfig {
  public:
    ACCConfig()
      : BaseConfig() {}
    virtual ~ACCConfig() {}
    virtual XMLNode MakeConfig(XMLNode cfg) const;
  };

} // namespace Arc

#endif // __ARC_ACC_H__
