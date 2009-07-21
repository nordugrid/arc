#ifndef __ARC_SEC_XACMLFUNCTIONFACTORY_H__
#define __ARC_SEC_XACMLFUNCTIONFACTORY_H__

#include <list>
#include <fstream>
#include <arc/security/ArcPDP/fn/FnFactory.h>

namespace ArcSec {

/// Function factory class for XACML specified attributes
class XACMLFnFactory : public FnFactory {
public:
  XACMLFnFactory();
  virtual ~XACMLFnFactory();

public:
  /**return a Function object according to the "Function" attribute in the XML node; 
  The XACMLFnFactory itself will release the Function objects*/
  virtual Function* createFn(const std::string& type);

private:
  void initFunctions();
};

Arc::Plugin* get_xacmlpdp_fn_factory (Arc::PluginArgument*);

} // namespace ArcSec

#endif /* __ARC_SEC_XACMLFUNCTIONFACTORY_H__ */

