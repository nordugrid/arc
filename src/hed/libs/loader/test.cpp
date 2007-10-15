#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>
#include "Loader.h"
#include <arc/ArcConfig.h>
#include <arc/Logger.h>

int main(void)
{
    Arc::LogStream ldest(std::cerr);
    Arc::Logger::getRootLogger().addDestination(ldest);
    Arc::Logger::getRootLogger().setThreshold(Arc::DEBUG);
    Arc::Config *c = new Arc::Config("test.xml");    
    Arc::Loader chains(c);
    return 0;
}
