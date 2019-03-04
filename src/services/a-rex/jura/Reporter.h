#ifndef _REPORTER_H
#define _REPORTER_H

#include <time.h>

#include <vector>
#include <string>

#include <arc/Logger.h>

#include "Destinations.h"

namespace ArcJura
{
  /** The class for main JURA functionality. Traverses the 'logs' dir
   *  of the given control directory, and reports usage data extracted from 
   *  job log files within.
   */
  class Reporter
  {
  public:
    /** Processes job log files in '<control_dir>/logs'. */
    virtual int report()=0;
    virtual ~Reporter() {};
  };

}

#endif
