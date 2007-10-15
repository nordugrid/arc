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
        XMLNode cn = (*cfg)["Handler"];
        Config cfg_(cn);
    }
}

} // namespace Arc
