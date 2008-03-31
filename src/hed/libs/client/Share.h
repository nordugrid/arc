/**
 * GLUE2 Share class
 */
#ifndef ARCLIB_SHARE
#define ARCLIB_SHARE

#include <string>

namespace Arc{

  class Share{
  protected:
    Share();

    std::string LocalID;
    std::string Name;
    std::string Description;

  public:
    virtual ~Share();
    

  }; //end class


}//end namespace

#endif
