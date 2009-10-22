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
 */
class RegisteredService: public Service
{
    private:
        InfoRegister inforeg;

    public:
        /** Example contructor - Server takes at least it's configuration subtree */
        RegisteredService(Config*);

        virtual ~RegisteredService(void) { };
};


} // namespace Arc

#endif /* __ARC_SERVICEISIS_H__ */
