#ifndef __ARC_FUNCTIONFACTORY_H__
#define __ARC_FUNCTIONFACTORY_H__

#include <map>
#include "../../../../libs/common/Logger.h"

namespace Arc {

typedef std::map<std::string, Arc::Function*> FnMap;

/** Base function factory class*/
cliass FnFactory {
public:
  FnFactory() {};
  virtual ~FnFactory();

public:
  virtual Function* createFn(const std::string& type){};

protected:
  FnMap fnmap;
};

} // namespace Arc

#endif /* __ARC_FUNCTIONFACTORY_H__ */

