#ifndef __ARC_SEC_ALGFACTORY_H__
#define __ARC_SEC_ALGFACTORY_H__

#include <arc/loader/LoadableClass.h>

#include <map>
#include "CombiningAlg.h"

namespace ArcSec {

typedef std::map<std::string, CombiningAlg*> AlgMap;

///Interface for algorithm factory class
/**AlgFactory is in charge of creating CombiningAlg according to the algorithm type*/
class AlgFactory : public Arc::LoadableClass {
public:
  AlgFactory() {};
  virtual ~AlgFactory() {};

public:
  /**creat algorithm object based on the type algorithm type*/
  virtual CombiningAlg* createAlg(const std::string& type) = 0;

protected:
  AlgMap algmap;
};

} // namespace ArcSec

#endif /* __ARC_SEC_ALGFACTORY_H__ */

