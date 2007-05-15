#include "ArcConfig.h"
#include <iostream>

int main(void)
{
    Arc::Config *config = new Arc::Config("test.xml");
    config->print();
    std::cout << (*config)["ArcConfig"]["ModuleLoader"]["Path"] << std::endl;
}
