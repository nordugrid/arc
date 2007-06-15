// Security.h

#ifndef __Security_h__
#define __Security_h__

#include "../../../libs/common/Logger.h"

namespace Arc{

  //! Common stuff used by security related slasses.
  /*! This class is just a place where to put common stuff that is
    used by security related slasses. So far it only contains a
    logger.
   */
  class Security {
  private:
    static Logger logger;
    friend class SecHandler;
    friend class PDP;
  };

}

#endif
