#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "grid_sched.h"

namespace GridScheduler {


Arc::MCC_Status GridSchedulerService::StopAcceptingNewActivities(Arc::XMLNode& in,Arc::XMLNode& out) {
  /*
  StopAcceptingNewActivities

  */

  IsAcceptingNewActivities = false;

  return Arc::MCC_Status(Arc::STATUS_OK);
}

} // namespace 

