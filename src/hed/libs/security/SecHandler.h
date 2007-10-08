#ifndef __ARC_SEC_SECHANDLER_H__
#define __ARC_SEC_SECHANDLER_H__

#include <arc/ArcConfig.h>
#include <arc/message/Message.h>
#include <arc/Logger.h>

namespace ArcSec {

//  class Arc::Config;
//  class Arc::Logger;
//  class Arc::Message;

  class SecHandler {
   public:
    SecHandler(Arc::Config*) {};
    virtual ~SecHandler() {};
    virtual bool Handle(Arc::Message *msg) = 0;
   protected:
    static Arc::Logger logger;
  };

} // namespace ArcSec

#endif /* __ARC_SEC_SECHANDLER_H__ */
