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
#include <arc/security/ArcPDP/PolicyStore.h>

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

  EvaluatorCombiningAlg combining_alg;

public:
  ArcEvaluator (Arc::XMLNode* cfg);
  ArcEvaluator (const char * cfgfile);
  virtual ~ArcEvaluator();

  /**Evaluate the request based on the policy information inside PolicyStore*/
  virtual Response* evaluate(Request* request);
  virtual Response* evaluate(Arc::XMLNode& node);
  virtual Response* evaluate(const char* reqfile);
  virtual Response* evaluate(std::string& reqstring);

  virtual Response* evaluate(Request* request, const char* policyfile);
  virtual Response* evaluate(Arc::XMLNode& node, const char* policyfile);
  virtual Response* evaluate(const char* reqfile, const char* policyfile);
  virtual Response* evaluate(std::string& reqstr, const char* policyfile);

  virtual Response* evaluate(Request* request, std::string& policystr);
  virtual Response* evaluate(Arc::XMLNode& node, std::string& policystr);
  virtual Response* evaluate(const char* reqfile, std::string& policystr);
  virtual Response* evaluate(std::string& reqstr, std::string& policystr);

  virtual Response* evaluate(Request* request, Arc::XMLNode& policynode);
  virtual Response* evaluate(Arc::XMLNode& node, Arc::XMLNode& policynode);
  virtual Response* evaluate(const char* reqfile, Arc::XMLNode& policynode);
  virtual Response* evaluate(std::string& reqstr, Arc::XMLNode& policynode);

  virtual Response* evaluate(Request* request, BasePolicy* policyobj);
  virtual Response* evaluate(Arc::XMLNode& node, BasePolicy* policyobj);
  virtual Response* evaluate(const char* reqfile, BasePolicy* policyobj);
  virtual Response* evaluate(std::string& reqstr, BasePolicy* policyobj);

  virtual AttributeFactory* getAttrFactory () { return attrfactory;};
  virtual FnFactory* getFnFactory () { return fnfactory; };
  virtual AlgFactory* getAlgFactory () { return algfactory; };

  virtual void addPolicy(const char* policyfile,const std::string& id = "") {
    plstore->addPolicy(policyfile, context, id);
  };

  virtual void addPolicy(std::string& policystr,const std::string& id = "") { 
    plstore->addPolicy(policystr, context, id);
  };

  virtual void addPolicy(Arc::XMLNode& policynode,const std::string& id = "") {
    plstore->addPolicy(policynode, context, id);
  };

  virtual void addPolicy(BasePolicy* policy,const std::string& id = "") { plstore->addPolicy(policy, context, id); };

  virtual void removePolicies(void) { plstore->removePolicies(); };

  virtual void setCombiningAlg(EvaluatorCombiningAlg alg);

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
