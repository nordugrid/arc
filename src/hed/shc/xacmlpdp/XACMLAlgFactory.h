#ifndef __ARC_SEC_XACMLALGFACTORY_H__
#define __ARC_SEC_XACMLALGFACTORY_H__

#include <list>
#include <fstream>
#include <arc/Logger.h>
#include <arc/security/ArcPDP/alg/AlgFactory.h>

namespace ArcSec {

///Algorithm factory class for XACML
class XACMLAlgFactory : public AlgFactory {
public:
  XACMLAlgFactory();
  virtual ~XACMLAlgFactory();

public:
  /**return a Alg object according to the "CombiningAlg" attribute in the <Policy> node;
  The XACMLAlgFactory itself will release the Alg objects*/
  virtual CombiningAlg* createAlg(const std::string& type);

private:
  void initCombiningAlg(CombiningAlg* alg);
  void initCombiningAlgs();
};

Arc::Plugin* get_xacmlpdp_alg_factory (Arc::PluginArgument*);

} // namespace ArcSec

#endif /* __ARC_SEC_XACMLALGFACTORY_H__ */

