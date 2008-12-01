#ifndef __ARC_ACC_H__
#define __ARC_ACC_H__

#include <arc/ArcConfig.h>
#include <arc/loader/Plugin.h>
#include <arc/credential/Credential.h>

namespace Arc {

  class ACC
    : public Plugin {
  protected:
    ACC(Config *cfg, const std::string& flavour = "");
  public:
    virtual ~ACC();
    const std::string& Flavour(){return flavour;}
  protected:
    std::string flavour;
    std::string proxyPath;
    std::string certificatePath;
    std::string keyPath;
    std::string caCertificatesDir;
    Arc::Credential* credential;
  };

  class ACCConfig
    : public BaseConfig {
  public:
    ACCConfig()
      : BaseConfig() {}
    virtual ~ACCConfig() {}
    virtual XMLNode MakeConfig(XMLNode cfg) const;
  };

  #define ACCPluginKind ("HED:ACC")

  class ACCPluginArgument: public PluginArgument {
   private:
    Config* config_;
   public:
    ACCPluginArgument(Config* config):config_(config) { };
    virtual ~ACCPluginArgument(void) { };
    operator Config* (void) { return config_; };
  };

} // namespace Arc

#endif // __ARC_ACC_H__
