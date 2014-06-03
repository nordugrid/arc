// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_CLIENTVOMSRESTFUL_H__
#define __ARC_CLIENTVOMSRESTFUL_H__

#include <string>
#include <arc/communication/ClientInterface.h>

namespace Arc {

  class ClientVOMSRESTful
    : public ClientHTTP {
  public:
    ClientVOMSRESTful() {}
    ClientVOMSRESTful(const BaseConfig& cfg, const std::string& host, int port, TCPSec sec, int timeout = -1, const std::string& proxy_host = "", int proxy_port = 0);
    virtual ~ClientVOMSRESTful();
    virtual MCC_Status Load();
    MCC_Status process(const std::list<std::string>& fqans,
                       const Period& lifetime,
                       std::string& result);
    MCC_Status process(const std::string& principal,
                       const std::list<std::string>& fqans,
                       const Period& lifetime,
                       const std::list<std::string>& targets,
                       std::string& result);
  };

} // namespace Arc

#endif // __ARC_CLIENTVOMSRESTFUL_H__
