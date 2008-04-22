/**
 * GLUE2 Contact_t class
 */
#ifndef GLUE2_CONTACT_T
#define GLUE2_CONTACT_T

#include "enums.h"
#include <arc/URL.h>
#include <string>

namespace Glue2{

  class Contact_t{
  public:

    Contact_t(){};
    ~Contact_t(){};

    std::string LocalID;
    Arc::URL URL;
    std::string Type;
    std::string OtherInfo;

  };//end class contact_t

} //end namespace

#endif
