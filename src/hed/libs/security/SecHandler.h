#ifndef __ARC_SECHANDLER_H__
#define __ARC_SECHANDLER_H__

namespace Arc {

  class Config;
  class Logger;
  class Message;

  class SecHandler {
   public:
    SecHandler(Config*) {};
    virtual ~SecHandler() {};
    virtual bool Handle(Message *msg) = 0;
   protected:
    static Logger logger;
  };

} // namespace Arc

#endif /* __ARC_SECHANDLER_H__ */
