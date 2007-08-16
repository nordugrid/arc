#ifndef __ARC_ALGFACTORY_H__
#define __ARC_ALGFACTORY_H__

#include <map>
#include "../../../../libs/common/Logger.h"

namespace Arc {

typedef std::map<std::string, Arc::CombiningAlg*> AlgMap;

/** Base algorithm factory class*/
cliass AlgFactory {
public:
  AlgFactory() {};
  virtual ~AlgFactory();

public:
  virtual CombiningAlg* createAlg(const std::string& type){};

protected:
  AlgMap algmap;
};

} // namespace Arc

#endif /* __ARC_ALGFACTORY_H__ */

