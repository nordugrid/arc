#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "MCC.h"

namespace Arc {

  Arc::Logger Arc::MCC::logger(Arc::Logger::rootLogger,"MCC");

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

void MCC::AddHandler(Config* cfg,Handler* handler,const std::string& label) {
    if(handler) {
    handlers_[label].push_back(handler); //need polishing to put the HandlerFactory->getinstance here
    XMLNode cn = (*cfg)["Handler"];
    printf("Handler here!!!!");
    Config cfg_(cn);
    handler->MakePDP(&cfg_);
    }
}

/*
void MCC::AuthN(AuthNHandler* authn,const std::string& label) {
    if(authn) authn_[label].push_back(authn);
}

void MCC::AuthZ(AuthZHandler* authz,const std::string& label) {
    if(authz) authz_[label].push_back(authz);
}
*/
}; // namespace Arc

