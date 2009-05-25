// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_ACC_H__
#define __ARC_ACC_H__

#include <arc/ArcConfig.h>
#include <arc/loader/Plugin.h>
#include <arc/message/MCC.h>

namespace Arc {

  class ACC
    : public Plugin {
  protected:
    ACC() {};
    ACC(Config *cfg, const std::string& flavour = "");
  public:
    virtual ~ACC();
    const std::string& Flavour() {
      return flavour;
    }

    //!Apply authentication credentials
    /*! This method applies the member credentials to the passed MCCConfig
        object reference.
     
       @param cfg The member credentials are applied to this object reference.
     */
    void ApplySecurity(MCCConfig& cfg) const {
      cfg.AddProxy(proxyPath);
      cfg.AddCertificate(certificatePath);
      cfg.AddPrivateKey(keyPath);
      cfg.AddCADir(caCertificatesDir);
    }
    
  protected:
    std::string flavour;
    std::string proxyPath;
    std::string certificatePath;
    std::string keyPath;
    std::string caCertificatesDir;
    int timeout;
  };

  class ACCConfig
    : public BaseConfig {
  public:
    ACCConfig()
      : BaseConfig() {}
    virtual ~ACCConfig() {}
    virtual XMLNode MakeConfig(XMLNode cfg) const;
    XMLNode MakeConfig() const {
      XMLNode cfg(NS(), "ArcConfig");
      MakeConfig(cfg);
      return cfg;
    }
  };

  #define ACCPluginKind ("HED:ACC")

  class ACCPluginArgument
    : public PluginArgument {
  public:
    ACCPluginArgument(Config *config)
      : config(config) {}
    virtual ~ACCPluginArgument() {}
    operator Config*() {
      return config;
    }
  private:
    Config *config;
  };

} // namespace Arc

#endif // __ARC_ACC_H__
