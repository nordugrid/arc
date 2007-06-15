#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "Service.h"

namespace Arc {

Logger Service::logger(Logger::rootLogger, "Service");

void Service::AddSecHandler(SecHandler* sechandler,const std::string& label) {
    if(sechandler) sechandlers_[label].push_back(sechandler);
}

} // namespace Arc
