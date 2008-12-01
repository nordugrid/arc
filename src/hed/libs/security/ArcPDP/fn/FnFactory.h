#ifndef __ARC_SEC_FUNCTIONFACTORY_H__
#define __ARC_SEC_FUNCTIONFACTORY_H__

#include <arc/security/ClassLoader.h>

#include <map>
#include "Function.h"

namespace ArcSec {

typedef std::map<std::string, Function*> FnMap;

///Interface for function factory class
/**FnFactory is in charge of creating Function object according to the 
 * algorithm type given as argument of method createFn. 
 * This class can be inherited for implementing a factory class
 * which can create some specific Function objects.
 */
class FnFactory : public Arc::LoadableClass {
public:
  FnFactory() {};
  virtual ~FnFactory(){};

public:
  /**creat algorithm object based on the type algorithm type
 *@param type  The type of Function
 *@return The object of Function
 */ 
  virtual Function* createFn(const std::string& type) = 0;

protected:
  FnMap fnmap;
};

} // namespace ArcSec

#endif /* __ARC_SEC_FUNCTIONFACTORY_H__ */

