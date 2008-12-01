#ifndef __ARC_SEC_ARCFUNCTIONFACTORY_H__
#define __ARC_SEC_ARCFUNCTIONFACTORY_H__

#include <list>
#include <fstream>
#include <arc/security/ArcPDP/fn/FnFactory.h>

namespace ArcSec {

/// Function factory class for Arc specified attributes
class ArcFnFactory : public FnFactory {
public:
  ArcFnFactory();
  virtual ~ArcFnFactory();

public:
  /**return a Function object according to the "Function" attribute in the XML node; 
  The ArcFnFactory itself will release the Function objects*/
  virtual Function* createFn(const std::string& type);

private:
  void initFunctions();
};

Arc::Plugin* get_arcpdp_fn_factory (Arc::PluginArgument*);

} // namespace ArcSec

#endif /* __ARC_SEC_ARCFUNCTIONFACTORY_H__ */

