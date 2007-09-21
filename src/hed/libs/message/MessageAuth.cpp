#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "MessageAuth.h"

namespace Arc {

void MessageAuth::set(const std::string& key __attribute__((unused)),
		      const AuthObject& value __attribute__((unused))) 
{
    // NOP
}

AuthObject MessageAuth::get(const std::string& key __attribute__((unused)),
			    int index __attribute__((unused)))
{
    return NULL;
}

void MessageAuth::remove(const std::string& key __attribute__((unused)))
{
    // NOP
}

} // namespace Arc
