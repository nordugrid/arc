#ifndef __ARC_SEC_FUNCTIONFACTORY_H__
#define __ARC_SEC_FUNCTIONFACTORY_H__

#include <arc/loader/LoadableClass.h>

#include <map>
#include "Function.h"

namespace ArcSec {

typedef std::map<std::string, Function*> FnMap;

/** Base function factory class*/
class FnFactory : public Arc::LoadableClass {
public:
  FnFactory() {};
  virtual ~FnFactory(){};

public:
  virtual Function* createFn(const std::string& type) = 0;

protected:
  FnMap fnmap;
};

} // namespace ArcSec

#endif /* __ARC_SEC_FUNCTIONFACTORY_H__ */

