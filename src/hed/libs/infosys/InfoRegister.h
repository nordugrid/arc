#ifndef __ARC_INFOSYS_REGISTER_H__
#define __ARC_INFOSYS_REGISTER_H__

#include <list>
#include <string>
#include <arc/ArcConfig.h>
#include <arc/XMLNode.h>
#include <arc/message/MCC.h>
#include <arc/message/Service.h>
#include <arc/client/ClientInterface.h>
#include <arc/Logger.h>
#include <arc/URL.h>

#define GLUE2_D42_NAMESPACE "http://schemas.ogf.org/glue/2008/05/spec_2.0_d42_r1"
#define REGISTRATION_NAMESPACE "http://www.nordugrid.org/schemas/registartion/2008/08"
#define ISIS_NAMESPACE "http://www.nordugrid.org/schemas/isis/2008/08"

namespace Arc
{

/// Registration to ISIS interface
/** This class provides an interface for service to register
  itself in Information Indexing Service. */
class InfoRegister
{
    private:
        std::list<Arc::URL> peers_;
        Arc::NS ns_;
        Arc::MCCConfig mcc_cfg_;
        Arc::ClientSOAP *cli_;
        Arc::Logger logger_;
        Arc::Service *service_;
        long int reg_period_;
    public:
        InfoRegister(Arc::XMLNode &node, Arc::Service *service_);
        ~InfoRegister();
        long int getPeriod(void) { return reg_period_; };
        void registration(void);

};

}
#endif
