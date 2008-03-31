/**
 * GLUE2 ComputingEndpoint class
 */
#ifndef ARCLIB_COMPUTINGENDPOINT
#define ARCLIB_COMPUTINGENDPOINT

#include "Endpoint.h"
#include "enums.h"
#include <string>

namespace Arc{

  class ComputingEndpoint : public Arc::Endpoint{
  public:
    ComputingEndpoint();
    ~ComputingEndpoint();

    Staging_t Staging;

  }; //end class

} //end namespace

#endif
