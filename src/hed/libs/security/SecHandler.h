#ifndef __ARC_SEC_SECHANDLER_H__
#define __ARC_SEC_SECHANDLER_H__

#include <arc/ArcConfig.h>
#include <arc/message/Message.h>
#include <arc/loader/Plugin.h>
#include <arc/Logger.h>

namespace Arc {
class ChainContext;
}

namespace ArcSec {

  /// Base class for simple security handling plugins
  /** This virtual class defines method Handle() which processes 
    security related information/attributes in Message and optionally
    makes security decision. Instances of such classes are normally
    arranged in chains abd are called on incoming and outgoing messages
    in various MCC and Service plugins. Return value of Handle() defines
    either processing should continie (true) or stop with error (false). 
    Configuration of SecHandler is consumed during creation of instance
    through XML subtree fed to constructor. */ 
  class SecHandler: public Arc::Plugin {
   public:
    SecHandler(Arc::Config*) {};
    virtual ~SecHandler() {};
    virtual bool Handle(Arc::Message *msg) = 0;

   protected:
    static Arc::Logger logger;
  };

  #define SecHandlerPluginKind ("HED:SHC")

  class SecHandlerPluginArgument: public Arc::PluginArgument {
   private:
    Arc::Config* config_;
    Arc::ChainContext* context_;
   public:
    SecHandlerPluginArgument(Arc::Config* config,Arc::ChainContext* context):config_(config),context_(context) { };
    virtual ~SecHandlerPluginArgument(void) { };
    operator Arc::Config* (void) { return config_; };
    operator Arc::ChainContext* (void) { return context_; };
  };

} // namespace ArcSec

#endif /* __ARC_SEC_SECHANDLER_H__ */
