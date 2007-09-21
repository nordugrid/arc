#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "MessageAuth.h"

namespace Arc {

void MessageAuth::set(const std::string&, const AuthObject&) 
{
    // NOP
}

AuthObject MessageAuth::get(const std::string&, int)
{
    return NULL;
}

void MessageAuth::remove(const std::string&)
{
    // NOP
}

} // namespace Arc
