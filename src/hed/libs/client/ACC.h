#ifndef __ARC_ACC_H__
#define __ARC_ACC_H__

#include <arc/ArcConfig.h>

namespace Arc {

  class ACC {
  protected:
    ACC(Config *cfg);
  public:
    virtual ~ACC();
  protected:
    std::string proxyPath;
    std::string certificatePath;
    std::string keyPath;
    std::string caCertificatesDir;
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
