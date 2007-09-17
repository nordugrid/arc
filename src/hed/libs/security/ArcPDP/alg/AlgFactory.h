#ifndef __ARC_ALGFACTORY_H__
#define __ARC_ALGFACTORY_H__

#include <arc/loader/LoadableClass.h>

#include <map>
#include "CombiningAlg.h"

namespace Arc {

typedef std::map<std::string, Arc::CombiningAlg*> AlgMap;

/** Base algorithm factory class*/
class AlgFactory : public LoadableClass {
public:
  AlgFactory() {};
  virtual ~AlgFactory() {};

public:
  virtual CombiningAlg* createAlg(const std::string& type){};

protected:
  AlgMap algmap;
};

} // namespace Arc

#endif /* __ARC_ALGFACTORY_H__ */

