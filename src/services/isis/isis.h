#ifndef __ARC_ISIS_H__
#define __ARC_ISIS_H__

#include <vector>
#include <arc/Logger.h>
#include <arc/XMLNode.h>
#include <arc/URL.h>
#include <arc/message/Service.h>
#include <arc/dbxml/XmlDatabase.h>
#include <arc/infosys/InfoRegister.h>
#include <arc/infosys/InformationInterface.h>


#include "router.h"

namespace ISIS
{

class ISIService: public Arc::Service
{
    private:
        std::string service_id_;
        Arc::XmlDatabase *db_;
        Arc::NS ns_;
        Arc::Logger logger_;
        std::vector<Arc::URL> peers_;
        Router router_;
		Arc::InformationContainer infodoc_;
        Arc::InfoRegister *reg_;
        Arc::MCC_Status make_soap_fault(Arc::Message &outmsg);
        bool loop_detection(Arc::XMLNode &in);
        // ISIS Interface
        Arc::MCC_Status Register(Arc::XMLNode &in, Arc::XMLNode &out);
        Arc::MCC_Status Query(Arc::XMLNode &in, Arc::XMLNode &out);
    public:
        ISIService(Arc::Config *cfg);
        virtual ~ISIService(void);
        virtual Arc::MCC_Status process(Arc::Message &in, 
                                        Arc::Message &out);
        virtual bool RegistrationCollector(Arc::XMLNode &doc);
        virtual std::string getID();

};

} // namespace

#endif
