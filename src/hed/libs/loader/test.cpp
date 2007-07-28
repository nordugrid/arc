#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>
#include "Loader.h"
#include "common/ArcConfig.h"

int main(void)
{
    Arc::Config *c = new Arc::Config("test.xml");    
    /* Arc::Loader *l = */ new Arc::Loader(c);
/*
    Arc::Config *c = new Arc::Config("test.xml");    
    Arc::Loader *l = new Arc::Loader(c);
    l->bootstrap();
    Loader::ModuleManager *mm = new Loader::ModuleManager();
    TestPlugin::TestClass *p = (TestPlugin::TestClass *)mm->load_mcc("libtestclass");
    printf("%p\n", p);
    // p->testfunc(5);
    // printf("test value: %d\n", p->a);
    p = (TestPlugin::TestClass *)mm->load_mcc("libtestclass");
    printf("%p\n", p);
    // printf("test value: %d\n", p->a);
    // std::cout << "Return: " << mm->load("libtestclass") << \
         " True: " << true << std::endl;
    // std::cout << "Return: " << mm->load("libtestclass") << \
         " True: " << true << std::endl;
    return 0;
*/

}
