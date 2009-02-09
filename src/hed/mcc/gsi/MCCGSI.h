#ifndef __ARC_MCCGSI_H__
#define __ARC_MCCGSI_H__

#include <gssapi.h>

#include <arc/message/MCC.h>

namespace Arc {

  class Logger;
  class PayloadGSIStream;

  class MCC_GSI_Service
    : public MCC {
  public:
    MCC_GSI_Service(Config& cfg,ModuleManager& mm);
    virtual ~MCC_GSI_Service();
    virtual MCC_Status process(Message&, Message&);
  private:
    std::string proxyPath;
    std::string certificatePath;
    std::string keyPath;
    static Logger logger;
  };

  class MCC_GSI_Client
    : public MCC {
  public:
    MCC_GSI_Client(Config& cfg,ModuleManager& mm);
    virtual ~MCC_GSI_Client();
    virtual MCC_Status process(Message&, Message&);
    virtual void Next(MCCInterface *next, const std::string& label = "");
  private:
    MCC_Status InitContext();
    gss_ctx_id_t ctx;
    std::string proxyPath;
    std::string certificatePath;
    std::string keyPath;
    static Logger logger;
  };

} // namespace Arc

#endif /* __ARC_MCCGSI_H__ */
