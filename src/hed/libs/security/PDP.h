#ifndef __ARC_PDP_H__
#define __ARC_PDP_H__

#include <string>
#include <arc/message/Message.h>

namespace Arc {

  class Config;
  class Logger;

  class PDP {
   public:
    PDP(Config*) {};
    virtual ~PDP() {};
    virtual bool isPermitted(Message *msg) = 0;
   protected:
    static Logger logger;
  };

} // namespace Arc

#endif /* __ARC_PDP_H__ */
