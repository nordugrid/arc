#ifndef __ARC_SEC_ALGFACTORY_H__
#define __ARC_SEC_ALGFACTORY_H__

#include <arc/loader/LoadableClass.h>

#include <map>
#include "CombiningAlg.h"

namespace ArcSec {

typedef std::map<std::string, CombiningAlg*> AlgMap;

/** Base algorithm factory class*/
class AlgFactory : public Arc::LoadableClass {
public:
  AlgFactory() {};
  virtual ~AlgFactory() {};

public:
  virtual CombiningAlg* createAlg(const std::string& type) = 0;

protected:
  AlgMap algmap;
};

} // namespace ArcSec

#endif /* __ARC_SEC_ALGFACTORY_H__ */

