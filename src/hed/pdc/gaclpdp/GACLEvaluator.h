#ifndef __ARC_SEC_GACLEVALUATOR_H__
#define __ARC_SEC_GACLEVALUATOR_H__

#include <arc/security/ArcPDP/Evaluator.h>
#include <arc/security/ArcPDP/PolicyStore.h>
/*
#include <list>
#include <fstream>

#include <arc/XMLNode.h>
#include <arc/Logger.h>
#include <arc/security/ArcPDP/fn/FnFactory.h>
#include <arc/security/ArcPDP/attr/AttributeFactory.h>
#include <arc/security/ArcPDP/alg/AlgFactory.h>
#include <arc/security/ArcPDP/Request.h>
#include <arc/security/ArcPDP/Response.h>
*/

namespace ArcSec {

class GACLEvaluator : public Evaluator {
private:
  static Arc::Logger logger;
  PolicyStore *plstore;

public:
  GACLEvaluator (Arc::XMLNode* cfg);
  GACLEvaluator (const char * cfgfile);
  virtual ~GACLEvaluator();

  /**Evaluate the request based on the policy information inside PolicyStore*/
  virtual Response* evaluate(Request* request);
  virtual Response* evaluate(const Source& request);

  virtual Response* evaluate(Request* request, const Source& policy);
  virtual Response* evaluate(const Source& request, const Source& policy);
  virtual Response* evaluate(Request* request, Policy* policyobj);
  virtual Response* evaluate(const Source& request, Policy* policyobj);

  virtual AttributeFactory* getAttrFactory () { return NULL; /*attrfactory;*/ };
  virtual FnFactory* getFnFactory () { return NULL; /*fnfactory;*/ };
  virtual AlgFactory* getAlgFactory () { return NULL; /*algfactory;*/ };

  virtual void addPolicy(const Source& policy,const std::string& id = "") {
    plstore->addPolicy(policy, NULL /* context */, id);
  };

  virtual void addPolicy(Policy* policy,const std::string& id = "") {
    plstore->addPolicy(policy, NULL /* context */, id);
  };

  virtual void removePolicies(void) { plstore->removePolicies(); };

  virtual void setCombiningAlg(EvaluatorCombiningAlg alg) { combining_alg = alg; } ;
  virtual void setCombiningAlg(CombiningAlg* alg) { } ;

  virtual const char* getName() const { return "gacl.evaluator"; };

  static Arc::LoadableClass* get_evaluator(void* arg);

protected:
  virtual Response* evaluate(EvaluationCtx* ctx) { };

private:
  virtual void parsecfg(Arc::XMLNode& cfg) { };
  EvaluatorCombiningAlg combining_alg;
};

} // namespace ArcSec

#endif /* __ARC_SEC_GACLEVALUATOR_H__ */

