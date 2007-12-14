#ifndef __ARC_SEC_PDP_H__
#define __ARC_SEC_PDP_H__

#include <string>
#include <arc/message/Message.h>
#include <arc/ArcConfig.h>
#include <arc/Logger.h>

namespace ArcSec {

  /// Base class for Policy Decisoion Point plugins
  /** This virtual class defines method isPermitted() which processes
    security related information/attributes in Message and makes security 
    decision - permit (true) or deny (false). 
    Configuration of PDP is consumed during creation of instance
    through XML subtree fed to constructor. */
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
