// Security.h

#ifndef __Security_h__
#define __Security_h__

#include <arc/Logger.h>

namespace ArcSec{

  //! Common stuff used by security related slasses.
  /*! This class is just a place where to put common stuff that is
    used by security related slasses. So far it only contains a
    logger.
   */
  class Security {
  private:
    static Arc::Logger logger;
    friend class SecHandler;
    friend class PDP;
  };

}

#endif
