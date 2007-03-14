#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>
#include "Plexer.h"

namespace Arc {

Plexer::Plexer(Arc::Config *cfg) : MCC(cfg)
{
}

Plexer::~Plexer(void) 
{
}

void Plexer::request(void)
{
    std::cout << "Plexer: request" << std::endl;

}

} // namespace Arc
