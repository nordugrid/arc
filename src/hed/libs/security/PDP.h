#ifndef __ARC_SEC_PDP_H__
#define __ARC_SEC_PDP_H__

#include <string>
#include <arc/message/Message.h>
#include <arc/ArcConfig.h>
#include <arc/Logger.h>

namespace ArcSec {

 // class Arc::Config;
 // class Arc::Logger;

  class PDP {
   public:
    PDP(Arc::Config*) {};
    virtual ~PDP() {};
    virtual bool isPermitted(Arc::Message *msg) = 0;
   protected:
    static Arc::Logger logger;
  };

} // namespace ArcSec

#endif /* __ARC_SEC_PDP_H__ */
