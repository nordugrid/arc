#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "grid_sched.h"


namespace GridScheduler {


Arc::MCC_Status GridSchedulerService::StartAcceptingNewActivities(Arc::XMLNode& in,Arc::XMLNode& out) {
  /*
  StartAcceptingNewActivities

  */

  IsAcceptingNewActivities = true;

  return Arc::MCC_Status(Arc::STATUS_OK);
}

} // namespace 

