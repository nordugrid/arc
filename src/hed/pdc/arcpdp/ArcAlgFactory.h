#ifndef __ARC_SEC_ARCALGFACTORY_H__
#define __ARC_SEC_ARCALGFACTORY_H__

#include <list>
#include <fstream>
#include <arc/Logger.h>
#include <arc/security/ArcPDP/alg/AlgFactory.h>

namespace ArcSec {

///Algorithm factory class for Arc
class ArcAlgFactory : public AlgFactory {
public:
  ArcAlgFactory();
  virtual ~ArcAlgFactory();

public:
  /**return a Alg object according to the "CombiningAlg" attribute in the <Policy> node;
  The ArcAlgFactory itself will release the Alg objects*/
  virtual CombiningAlg* createAlg(const std::string& type);

private:
  void initCombiningAlg(CombiningAlg* alg);
  void initCombiningAlgs();
};

} // namespace ArcSec

#endif /* __ARC_SEC_ARCALGFACTORY_H__ */

