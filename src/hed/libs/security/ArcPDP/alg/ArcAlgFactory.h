#ifndef __ARC_ARCALGFACTORY_H__
#define __ARC_ARCALGFACTORY_H__

#include <list>
#include <fstream>
#include <arc/Logger.h>
#include "AlgFactory.h"

namespace Arc {

/** Algorithm factory class for Arc*/
class ArcAlgFactory : public AlgFactory {
public:
  ArcAlgFactory();
  virtual ~ArcAlgFactory();

public:
  virtual CombiningAlg* createAlg(const std::string& type);

private:
  void initCombiningAlgs();
};

} // namespace Arc

#endif /* __ARC_ARCALGFACTORY_H__ */

