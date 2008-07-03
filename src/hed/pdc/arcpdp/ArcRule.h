#ifndef __ARC_SEC_ARCRULE_H__
#define __ARC_SEC_ARCRULE_H__

#include <arc/XMLNode.h>
#include <list>

#include <arc/security/ArcPDP/policy/Policy.h>
#include <arc/security/ArcPDP/fn/Function.h>
#include <arc/security/ArcPDP/alg/CombiningAlg.h>
#include <arc/security/ArcPDP/attr/AttributeFactory.h>
#include <arc/security/ArcPDP/fn/FnFactory.h>
#include <arc/security/ArcPDP/Evaluator.h>

namespace ArcSec {
///pair Match include the AttributeValue object in <Rule> and the Function which is used to handle the AttributeValue,
///default function is "Equal", if some other function is used, it should be explicitly specified, e.g.
///<Subject Type="string" Function="Match">/vo.knowarc/usergroupA</Subject>
typedef std::pair<AttributeValue*, Function*> Match;
 
/**<Subjects> example inside <Rule>:
      <Subjects>
         <Subject Type="X500Name">/O=NorduGrid/OU=UIO/CN=test</Subject>
         <Subject Type="string">/vo.knowarc/usergroupA</Subject>
         <Subject>
            <SubFraction Type="string">/O=Grid/OU=KnowARC/CN=XYZ</SubFraction>
            <SubFraction Type="string">urn:mace:shibboleth:examples</SubFraction>
         </Subject>
         <GroupIdRef Location="./subjectgroup.xml">subgrpexample1</GroupIdRef>
      </Subjects>*/

/** "And" relationship means the request should satisfy all of the items
  <Subject>
   <SubFraction Type="X500DN">/O=Grid/OU=KnowARC/CN=XYZ</SubFraction>
   <SubFraction Type="ShibName">urn:mace:shibboleth:examples</SubFraction>
  </Subject>
  */

/** "Or" relationship meand the request should satisfy any of the items
  <Subjects>
    <Subject Type="X500DN">/O=Grid/OU=KnowARC/CN=ABC</Subject>
    <Subject Type="VOMSAttribute">/vo.knowarc/usergroupA</Subject>
    <Subject>
      <SubFraction Type="X500DN">/O=Grid/OU=KnowARC/CN=XYZ</SubFraction>
      <SubFraction Type="ShibName">urn:mace:shibboleth:examples</SubFraction>
    </Subject>
    <GroupIdRef Location="./subjectgroup.xml">subgrpexample1</GroupIdRef>
  </Subjects>
*/

///AndList - include items inside one <Subject> (or <Resource> <Action> <Condition>)
typedef std::list<Match> AndList;

///OrList  - include items inside one <Subjects> (or <Resources> <Actions> <Conditions>)
typedef std::list<AndList> OrList;


enum Id_MatchResult {
  //The "id" of all the <Attribute>s under a <Subject> (or other type) is matched
  //by <Attribute>s under <Subject> in <RequestItem>
  ID_MATCH = 0,
  //Part "id" is matched
  ID_PARTIAL_MATCH = 1,
  //Any "id" of the <Attrubute>s is not matched
  ID_NO_MATCH = 2
};

///ArcRule class to parse Arc specific <Rule> node
class ArcRule : public Policy {
public:
  ArcRule(Arc::XMLNode* node, EvaluatorContext* ctx);  

  virtual std::string getEffect();

  virtual Result eval(EvaluationCtx* ctx);

  virtual MatchResult match(EvaluationCtx* ctx);

  virtual ~ArcRule();

  virtual EvalResult& getEvalResult();

  virtual void setEvalResult(EvalResult& res);

private:
  /**Parse the <Subjects> <Resources> <Actions> <Conditions> inside one <Rule>
  Can also refer to the other source by using <GroupIdRef>, the <Location> attribute is the location of the refered file
  the value "subgrpexample" is the index for searching in the refered file
  */
  void getItemlist(Arc::XMLNode& nd, OrList& items, const std::string& itemtype, const std::string& type_attr, 
    const std::string& function_attr);

private:
  std::string effect;
  std::string id;
  std::string version;
  std::string description;
 
  OrList subjects;
  OrList resources;
  OrList actions;
  OrList conditions;

  AttributeFactory* attrfactory;
  FnFactory* fnfactory;

  EvalResult evalres;
  Arc::XMLNode rulenode;

  Id_MatchResult sub_idmatched;
  Id_MatchResult res_idmatched;
  Id_MatchResult act_idmatched;
  Id_MatchResult ctx_idmatched; 

protected:
  static Arc::Logger logger;
};

} // namespace ArcSec

#endif /* __ARC_SEC_ARCRULE_H__ */

