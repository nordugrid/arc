/**
 * GLUE2 Activity class
 */
#ifndef ARCLIB_ACTIVITY
#define ARCLIB_ACTIVITY

#include <arc/URL.h>

namespace Arc{

  //open enumeration
  enum ActivityType_t{computing};
  
  
  class Activity{
  protected:
    Activity();

    Arc::URL ID;
    ActivityType_t Type;

  public:
    virtual ~Activity();


  };//end class endpoint

} //end namespace
 
#endif
