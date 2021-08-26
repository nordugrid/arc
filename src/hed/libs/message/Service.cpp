#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "Service.h"

namespace Arc {

Logger Service::logger(Logger::getRootLogger(), "Service");

Service::Service(Config*, PluginArgument* arg) : MCCInterface(arg), valid(true) {
}

void Service::AddSecHandler(Config* cfg,ArcSec::SecHandler* sechandler,const std::string& label) {
    if(sechandler) {
        sechandlers_[label].push_back(sechandler); //need polishing to put the SecHandlerFactory->getinstance here
        XMLNode cn = (*cfg)["SecHandler"];
        Config cfg_(cn);
    }
}

MCC_Status Service::ProcessSecHandlers(Message& message,const std::string& label) const {
    std::map<std::string,std::list<ArcSec::SecHandler*> >::const_iterator q = sechandlers_.find(label);
    if(q == sechandlers_.end()) {
        logger.msg(DEBUG, "No security processing/check requested for '%s'", label);
        return MCC_Status(STATUS_OK);
    }

    std::list<ArcSec::SecHandler*>::const_iterator h = q->second.begin();
    for(;h!=q->second.end();++h) {
        const ArcSec::SecHandler* handler = *h;
        if(handler) {
            ArcSec::SecHandlerStatus ret = handler->Handle(&message);
            if(!ret) {
                logger.msg(DEBUG, "Security processing/check for '%s' failed: %s", label, (std::string)ret);
                return MCC_Status(GENERIC_ERROR, ret.getOrigin(),
                           ret.getExplanation().empty()?(std::string("Security error: ")+Arc::tostring(ret.getCode())):ret.getExplanation());
            }
        }
    }
    logger.msg(DEBUG, "Security processing/check for '%s' passed", label);
    return MCC_Status(STATUS_OK);
}

} // namespace Arc
