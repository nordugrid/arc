/**
 * GLUE2 Activity class
 */
#ifndef GLUE2_ACTIVITY_T
#define GLUE2_ACTIVITY_T

#include "enums.h"
#include <arc/URL.h>

namespace Glue2{

  class Activity_t{
  protected:
    Activity_t(){};

    Arc::URL ID;
    ActivityType_t Type;

  public:
    virtual ~Activity_t(){};


  };//end class endpoint

} //end namespace
 
#endif
