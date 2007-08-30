#ifndef __ARC_ARCFUNCTIONFACTORY_H__
#define __ARC_ARCFUNCTIONFACTORY_H__

#include <list>
#include <fstream>
#include "FnFactory.h"

namespace Arc {

/** Function factory class for Arc specified attributes*/
class ArcFnFactory : public FnFactory {
public:
  ArcFnFactory();
  virtual ~ArcFnFactory();

public:
  virtual Function* createFn(const std::string& type);

private:
  void initFunctions();
};

} // namespace Arc

#endif /* __ARC_ARCFUNCTIONFACTORY_H__ */

