#ifndef __ARC_SEC_ARCEVALUATOR_H__
#define __ARC_SEC_ARCEVALUATOR_H__

#include <list>
#include <fstream>
#include <arc/XMLNode.h>
#include <arc/Logger.h>

#include <arc/security/ArcPDP/Evaluator.h>
#include <arc/security/ArcPDP/fn/FnFactory.h>
#include <arc/security/ArcPDP/attr/AttributeFactory.h>
#include <arc/security/ArcPDP/alg/AlgFactory.h>
#include <arc/security/ArcPDP/Request.h>
#include <arc/security/ArcPDP/Response.h>
#include "PolicyStore.h"

namespace ArcSec {

///Execute the policy evaluation, based on the request and policy
class ArcEvaluator : public Evaluator {
friend class EvaluatorContext;
private:
  static Arc::Logger logger;
  PolicyStore *plstore;
  FnFactory* fnfactory;
  AttributeFactory* attrfactory;  
  AlgFactory* algfactory;
  
  EvaluatorContext* context;

  Arc::XMLNode* m_cfg;
  std::string request_classname;

public:
  ArcEvaluator (Arc::XMLNode* cfg);
  ArcEvaluator (const char * cfgfile);
  virtual ~ArcEvaluator();

  /**Evaluate the request based on the policy information inside PolicyStore*/
  virtual Response* evaluate(Request* request);
  virtual Response* evaluate(Arc::XMLNode& node);
  virtual Response* evaluate(const std::string& reqfile);

  virtual AttributeFactory* getAttrFactory () { return attrfactory;};
  virtual FnFactory* getFnFactory () { return fnfactory; };
  virtual AlgFactory* getAlgFactory () { return algfactory; };

protected:
  virtual Response* evaluate(EvaluationCtx* ctx);

private:
  /**Parse the configure information about PDP, then the related object can be loaded according to the configure information
       <pdp:PDPConfig>
          <pdp:PolicyStore name="test" location="Policy_Example.xml"/>
          <pdp:AttributeFactory name="attr.factory" />
          <pdp:CombingAlgorithmFactory name="alg.factory" />
          <pdp:FunctionFactory name="fn.factory" />
          <pdp:Evaluator name="arc.evaluator" />
          <pdp:Request name="arc.request" />
       </pdp:PDPConfig>
  */
  virtual void parsecfg(Arc::XMLNode& cfg);
  virtual Request* make_reqobj(Arc::XMLNode& reqnode);
};

} // namespace ArcSec

#endif /* __ARC_SEC_ARCEVALUATOR_H__ */
