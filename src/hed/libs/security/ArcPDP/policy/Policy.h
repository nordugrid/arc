#ifndef __ARC_SEC_POLICY_H__
#define __ARC_SEC_POLICY_H__

#include <list>
#include <arc/XMLNode.h>
#include <arc/Logger.h>
#include "../Result.h"
#include "../EvaluationCtx.h"


namespace ArcSec {

///Base class for Policy, PolicySet, or Rule
/*Basically, each policy object is a container which includes a few elements 
e.g., ArcPolicySet objects includes a few ArcPolicy objects; ArcPolicy object includes a few ArcRule objects.
there is logical relationship between ArcRules or ArcPolicies, which is called combining algorithm. According to algorithm, 
evaluation results from the elements are combined, and then the combined evaluation result is returned to the up-level. 
*/
class Policy {
protected:
  std::list<Policy*> subelements;
  static Arc::Logger logger; 
 
public:
  Policy(Arc::XMLNode) {};  
  virtual ~Policy(){};
  
  ///Evaluate whether the two targets to be evaluated match to each other
  /**As an example for illustration, for the ArcRule, the rule is like this:
   <Rule RuleId="rule2" Effect="Deny">
      <Subjects>
         <Subject Type="string">/O=Grid/OU=KnowARC/CN=ANONYMOS</Subject>
         <Subject Type="string">/vo.knowarc/usergroupB</Subject>
      </Subjects>
      <Resources Type="string">
         <Resource>localhost:/home/atlas/</Resource>
         <Resource>nordugrid.org:/home/atlas/</Resource>
      </Resources>
      <Actions Type="string">
         <Action>read</Action>
      </Actions>
      <Conditions/>
   </Rule>
   the match(ctx) method will check whether the Request (with Arc request schema) satisfies the <Subjects, Resources, Actions, Conditions> tuple.

   for the XACML rule, Rule is like this:
   <Rule RuleId="urn:oasis:names:tc:xacml:2.0:example:ruleid:2" Effect="Permit">
    <Target>
      <Resources>
        <Resource>
          <ResourceMatch MatchId="urn:oasis:names:tc:xacml:1.0:function:string-equal">
             <AttributeValue DataType="http://www.w3.org/2001/XMLSchema#string">urn:med:example:schemas:record</AttributeValue>
             <ResourceAttributeDesignator AttributeId="urn:oasis:names:tc:xacml:2.0:resource:target-namespace" DataType="http://www.w3.org/2001/XMLSchema#string"/>
          </ResourceMatch>
        </Resource>
      </Resources>
      <Actions>
        <Action>
          <ActionMatch MatchId="urn:oasis:names:tc:xacml:1.0:function:string-equal">
            <AttributeValue DataType="http://www.w3.org/2001/XMLSchema#string">read</AttributeValue>
            <ActionAttributeDesignator AttributeId="urn:oasis:names:tc:xacml:1.0:action:action-id" DataType="http://www.w3.org/2001/XMLSchema#string"/>
          </ActionMatch>
        </Action>
      </Actions>
    </Target>
    <Condition>
      <Apply FunctionId="urn:oasis:names:tc:xacml:1.0:function:and">
        <Apply FunctionId="urn:oasis:names:tc:xacml:1.0:function:string-equal">
          <Apply FunctionId="urn:oasis:names:tc:xacml:1.0:function:string-one-and-only">
            <SubjectAttributeDesignator AttributeId="urn:oasis:names:tc:xacml:2.0:example:attribute:parent-guardian-id" DataType="http://www.w3.org/2001/XMLSchema#string"/>
          </Apply>
          <Apply FunctionId="urn:oasis:names:tc:xacml:1.0:function:string-one-and-only">
            <AttributeSelector RequestContextPath="//md:record/md:parentGuardian/md:parentGuardianId/text()" DataType="http://www.w3.org/2001/XMLSchema#string"/>
          </Apply>
        </Apply>
        <VariableReference VariableId="17590035"/>
      </Apply>
    </Condition>
   </Rule>
   the match(ctx) method will check whether the Request (with XAMCL request schema) satisfies the <Target> tuple (which include 
   <Subjects, Resources, Actions>)
   
  */   
  virtual MatchResult match(EvaluationCtx* ctx) = 0;

  ///Evaluate policy
  /*For the <Rule> of Arc, only get the "Effect" from rules; 
    For the <Policy< of Arc, combine the evaluation result from <Rule>
    
    For the <Rule> of XACML, it will evaluate the <Condition> node by using information from request, and use the "Effect" attribute of <Rule>
    For the <Policy> of XACML, it will combine the evaluation result from <Rule>
  */
  virtual Result eval(EvaluationCtx* ctx) = 0;

  ///Add a policy element to into "this" object
  virtual void addPolicy(Policy* pl){subelements.push_back(pl);};

  ///Get the "Effect" attribute
  virtual std::string getEffect() = 0;
  
  ///Get eveluation result
  virtual EvalResult& getEvalResult() = 0;

};

} // namespace ArcSec

#endif /* __ARC_SEC_POLICY_H__ */

