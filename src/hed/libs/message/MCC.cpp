#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "MCC.h"

namespace Arc {

Arc::Logger Arc::MCC::logger(Arc::Logger::getRootLogger(),"MCC");

void MCC::Next(MCCInterface* next,const std::string& label) {
    if(next == NULL) {
        next_.erase(label);
    } else {
        next_[label]=next;
    };
}

MCCInterface* MCC::Next(const std::string& label) {
    std::map<std::string,MCCInterface*>::iterator n = next_.find(label);
    if(n == next_.end()) return NULL;
    return n->second;
}

void MCC::Unlink(void) {
    for(std::map<std::string,MCCInterface*>::iterator n = next_.begin();
                                     n != next_.end();n = next_.begin()) next_.erase(n);
}

void MCC::AddSecHandler(Config* cfg, ArcSec::SecHandler* sechandler,const std::string& label) {
    if(sechandler) {
        sechandlers_[label].push_back(sechandler); //need polishing to put the SecHandlerFactory->getinstance here
        XMLNode cn = (*cfg)["SecHandler"];
        Config cfg_(cn);
    }
}

bool MCC::ProcessSecHandlers(Arc::Message& message,const std::string& label) {
    //Each MCC/Service can define security handler queues in the configuration
    // file, the queues have labels specified in handlers configuration 'event'
    // attribute.
    // Security handlers in one queue are called sequentially. 
    // Each one should be configured carefully, because there can be some relationship 
    // between them (e.g. authentication should be put in front of authorization).
    // The SecHandler::Handle() only returns true/false with true meaning that handler
    // took positiove decision and there is no need continue executing other handlers.
    // If any SecHandler in the handler chain produces some information which will be 
    // used by some following handler, the information should be stored in the
    // attributes of message (e.g. the Identity extracted from authentication will
    // be used by authorization to make access control decision).
    std::map<std::string,std::list<ArcSec::SecHandler*> >::iterator q = sechandlers_.find(label);
    if(q == sechandlers_.end()) {
        logger.msg(Arc::VERBOSE, "No security processing/check required");
        if(sechandlers_.size())
          logger.msg(Arc::VERBOSE, "There is security handling events inside the Service, please make sure you set the label parameters when you call the ProcessSecHandler method");
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

} // namespace Arc
