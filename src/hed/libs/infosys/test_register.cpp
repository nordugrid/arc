#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/ArcConfig.h>
#include <arc/infosys/InfoRegister.h>

int main(void)
{
    Arc::Config *cfg = new Arc::Config("service.xml");
    Arc::InfoRegister *r;
    r = new Arc::InfoRegister("a", 60, *cfg); 
}
