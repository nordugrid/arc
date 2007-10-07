#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "ArcConfig.h"
#include "Thread.h"
#include <iostream>

void func(void* arg) {
    std::cout << "Thread argument = " << arg << std::endl;
}

int main(void)
{
    Arc::Config *config = new Arc::Config("test.xml");
    config->print();
    std::cout << "ArcConfig:ModuleLoader:Path = " << (*config)["ArcConfig"]["ModuleLoader"]["Path"] << std::endl;

    Arc::CreateThreadFunction(func,(void*)5);
    sleep(1);

    return 0;
}
