// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_CLIENTVOMS_H__
#define __ARC_CLIENTVOMS_H__

#include <string>
#include <arc/communication/ClientInterface.h>

namespace Arc {

  class VOMSCommand {
   //  According to https://wiki.italiangrid.it/twiki/bin/view/VOMS/VOMSProtocol
   //  Note that List andd Query commands are deprecated
   // and hence not present here. 
   private:
    std::string str;
   public:
    VOMSCommand(void) {};
    ~VOMSCommand(void) {};
    VOMSCommand& GetGroup(const std::string& groupname);
    VOMSCommand& GetRole(const std::string& rolename);
    VOMSCommand& GetRoleInGroup(const std::string& groupname, const std::string& rolename);
    VOMSCommand& GetEverything(void);
    VOMSCommand& GetFQANs(void);
    VOMSCommand& GetFQAN(const std::string& fqan);
    const std::string& Str(void) const { return str; };
    operator const std::string&(void) const { return str; };
  };

  class ClientVOMS
    : public ClientTCP {
  public:
    ClientVOMS() {}
    ClientVOMS(const BaseConfig& cfg, const std::string& host, int port, TCPSec sec, int timeout = -1);
    virtual ~ClientVOMS();
    virtual MCC_Status Load();
    MCC_Status process(const VOMSCommand& command,
                       const Period& lifetime,
                       std::string& result);
    MCC_Status process(const VOMSCommand& command,
                       const std::list<std::pair<std::string,std::string> >& order,
                       const Period& lifetime,
                       std::string& result);
    MCC_Status process(const std::list<VOMSCommand>& commands,
                       const Period& lifetime,
                       std::string& result);
    MCC_Status process(const std::list<VOMSCommand>& commands,
                       const std::list<std::pair<std::string,std::string> >& order,
                       const Period& lifetime,
                       std::string& result);
  };

} // namespace Arc

#endif // __ARC_CLIENTVOMS_H__
