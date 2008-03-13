#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/ArcConfig.h>
#include <arc/infosys/Register.h>

int main(void)
{
    Arc::Config *cfg = new Arc::Config("service.xml");
    Arc::Register *r;
    r = new Arc::Register("a", 60, *cfg); 
}
