#ifndef __ARC_SERVICEISIS_H__
#define __ARC_SERVICEISIS_H__

#include <arc/message/Service.h>
#include <arc/ArcConfig.h>
#include <arc/Logger.h>
#include <arc/message/MCC.h>
#include <arc/loader/Plugin.h>
#include <arc/infosys/InfoRegister.h>

namespace Arc {

/// RegisteredService - extension of Service performing self-registration.
/**
   Service is automatically added to registration framework. Registration
   information for service is obtained by calling its RegistrationCollector()
   method. It is important to note that RegistrationCollector() may be called 
   anytime after RegisteredService constructor completed and hence even before
   actual constructor of inheriting class is complete. That must be taken into
   account while writing implementation of RegistrationCollector() or object of
   InfoRegisters class must be used directly.
 */
class RegisteredService: public Service
{
    private:
        InfoRegisters inforeg;

    public:
        /** Example contructor - Server takes at least it's configuration subtree */
        RegisteredService(Config*, PluginArgument*);

        virtual ~RegisteredService(void);
};


} // namespace Arc

#endif /* __ARC_SERVICEISIS_H__ */
