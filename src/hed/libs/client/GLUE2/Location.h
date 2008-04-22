/**
 * GLUE2 Location class
 */
#ifndef GLUE2_LOCATION_T
#define GLUE2_LOCATION_T

#include <string>

namespace Glue2{
        
  class Location_t{
  public:
    Location_t(){};
    ~Location_t(){};

    std::string Name;
    std::string Owner;
    std::string Address;
    std::string Place;
    std::string Country;
    std::string PostCode;
    float Latitude;
    float Longitude;


  }; //end class Location_t

} //end namespace

#endif
