#ifndef __ARC_FUNCTIONFACTORY_H__
#define __ARC_FUNCTIONFACTORY_H__

#include <arc/loader/LoadableClass.h>

#include <map>
#include "Function.h"

namespace Arc {

typedef std::map<std::string, Arc::Function*> FnMap;

/** Base function factory class*/
class FnFactory : public LoadableClass {
public:
  FnFactory() {};
  virtual ~FnFactory(){};

public:
  virtual Function* createFn(const std::string& type) = 0;

protected:
  FnMap fnmap;
};

} // namespace Arc

#endif /* __ARC_FUNCTIONFACTORY_H__ */

