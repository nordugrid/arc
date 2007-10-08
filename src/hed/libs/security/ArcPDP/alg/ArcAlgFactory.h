#ifndef __ARC_SEC_ARCALGFACTORY_H__
#define __ARC_SEC_ARCALGFACTORY_H__

#include <list>
#include <fstream>
#include <arc/Logger.h>
#include <arc/security/ArcPDP/alg/AlgFactory.h>

namespace ArcSec {

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

} // namespace ArcSec

#endif /* __ARC_SEC_ARCALGFACTORY_H__ */

