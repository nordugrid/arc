/**
 * GLUE2 ApplicationEnvironment class
 */
#ifndef GLUE2_APPLICATIONENVIRONMENT_T
#define GLUE2_APPLICATIONENVIRONMENT_T

#include "enums.h"
#include <string>

namespace Glue2{

  class ApplicationEnvironment_t{
  public:
    ApplicationEnvironment_t(){};
    ~ApplicationEnvironment_t(){};

    std::string LocalID;
    std::string Name;
    std::string Version;
    AppEnvState_t State;
    int LifeTime;
    License_t License;
    std::string Description;
    ParallelType_t ParallelType;
    int MaxSlots;
    int MaxJobs;
    int MaxUserSeats;
    int FreeSlots;
    int FreeJobs;
    int FreeUserSeats;
    std::list<ApplicationHandle_t> ApplicationHandle;
    std::list<std::string> ExecutionEnvironmentLocalID;

  };//end class applicationenvironment

}//end namespace

#endif
