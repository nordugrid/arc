/**
 * GLUE2 ComputingEndpoint class
 */
#ifndef GLUE2_COMPUTINGENDPOINT_T
#define GLUE2_COMPUTINGENDPOINT_T

#include "Endpoint.h"
#include "enums.h"
#include <string>

namespace Glue2{

  class ComputingEndpoint_t : public Glue2::Endpoint_t{
  public:
    ComputingEndpoint_t(){};
    ~ComputingEndpoint_t(){};

    Staging_t Staging;

  }; //end class

} //end namespace

#endif
