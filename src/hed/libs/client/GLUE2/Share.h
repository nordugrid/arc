/**
 * GLUE2 Share class
 */
#ifndef GLUE2_SHARE_T
#define GLUE2_SHARE_T

#include <string>

namespace Glue2{

  class Share_t{

  public:
    Share_t(){};
    virtual ~Share_t(){};

    std::string LocalID;
    std::string Name;
    std::string Description;    

  }; //end class


}//end namespace

#endif
