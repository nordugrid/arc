#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "MessageAuth.h"

namespace Arc {

void MessageAuth::set(const std::string& key, const AuthObject& value) 
{
    // NOP
}

AuthObject MessageAuth::get(const std::string& key, int index)
{
    return NULL;
}

void MessageAuth::remove(const std::string& key)
{
    // NOP
}

}; // namespace Arc
