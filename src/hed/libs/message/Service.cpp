#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "Service.h"

namespace Arc {

Logger Service::logger(Logger::getRootLogger(), "Service");

void Service::AddSecHandler(Config* cfg,ArcSec::SecHandler* sechandler,const std::string& label) {
    if(sechandler) {
        sechandlers_[label].push_back(sechandler); //need polishing to put the SecHandlerFactory->getinstance here
        XMLNode cn = (*cfg)["SecHandler"];
        Config cfg_(cn);
    }
}

bool Service::ProcessSecHandlers(Arc::Message& message,const std::string& label) {
    std::map<std::string,std::list<ArcSec::SecHandler*> >::iterator q = sechandlers_.find(label);
    if(q == sechandlers_.end()) {
        logger.msg(Arc::VERBOSE, "No security processing/check requested for '%s'", label);
        return true;
    }

    std::list<ArcSec::SecHandler*>::iterator h = q->second.begin();
    for(;h!=q->second.end();++h) {
        ArcSec::SecHandler* handler = *h;
        if(handler) if(handler->Handle(&message)) {
            logger.msg(Arc::VERBOSE, "Security processing/check passed");
            return true;
        }
    }
    logger.msg(Arc::VERBOSE, "Security processing/check failed");
    return false;
}

bool Service::RegistrationCollector(Arc::XMLNode &doc)
{
    logger.msg(Arc::WARNING, "Empty registration collector");
}

} // namespace Arc
