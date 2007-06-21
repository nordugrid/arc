#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "Service.h"

namespace Arc {

Logger Service::logger(Logger::rootLogger, "Service");

void Service::AddSecHandler(Config* cfg,SecHandler* sechandler,const std::string& label) {
    if(sechandler) {
        sechandlers_[label].push_back(sechandler); //need polishing to put the SecHandlerFactory->getinstance here
        XMLNode cn = (*cfg)["Handler"];
        Config cfg_(cn);
    }
}

} // namespace Arc
