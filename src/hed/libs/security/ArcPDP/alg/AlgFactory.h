#ifndef __ARC_SEC_ALGFACTORY_H__
#define __ARC_SEC_ALGFACTORY_H__

#include <arc/security/ClassLoader.h>

#include <map>
#include "CombiningAlg.h"

namespace ArcSec {

typedef std::map<std::string, CombiningAlg*> AlgMap;

///Interface for algorithm factory class
/**AlgFactory is in charge of creating CombiningAlg according to the algorithm type given as 
 * argument of method createAlg. This class can be inherited for implementing a factory class
 * which can create some specific combining algorithm objects. 
 */
class AlgFactory : public Arc::LoadableClass {
public:
  AlgFactory() {};
  virtual ~AlgFactory() {};

public:
  /**creat algorithm object based on the type algorithm type
  *@param type  The type of combining algorithm
  *@return The object of CombiningAlg 
  */
  virtual CombiningAlg* createAlg(const std::string& type) = 0;

protected:
  AlgMap algmap;
};

} // namespace ArcSec

#endif /* __ARC_SEC_ALGFACTORY_H__ */

