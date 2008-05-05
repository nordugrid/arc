#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "ArcLocation.h"
#include "ArcConfig.h"
#include "Thread.h"
#include <iostream>
#include <unistd.h>

#if 0
void func(void* arg) {
    std::cout << "Thread argument = " << arg << std::endl;
}
#endif

int main(int argc, char **argv)
{

    Arc::ArcLocation::Init(".");
#if 0
    Arc::Config *config = new Arc::Config("test.xml");
    config->print();
    std::cout << "ArcConfig:ModuleLoader:Path = " << (*config)["ArcConfig"]["ModuleLoader"]["Path"] << std::endl;

    Arc::CreateThreadFunction(func,(void*)5);
#ifndef WIN32
    sleep(1);
#endif
#endif
    return 0;
}
