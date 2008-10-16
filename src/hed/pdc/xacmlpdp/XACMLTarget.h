#ifndef __ARC_SEC_XACMLTARGET_H__
#define __ARC_SEC_XACMLTARGET_H__

#include <list>
#include <arc/XMLNode.h>
#include <arc/security/ArcPDP/fn/Function.h>
#include <arc/security/ArcPDP/attr/AttributeFactory.h>
#include <arc/security/ArcPDP/fn/FnFactory.h>
#include <arc/security/ArcPDP/Evaluator.h>

#include <./AttributeSelector.h>
#include <./AttributeDesginator.h>

namespace ArcSec {

//<SubjectMatch/> <ResourceMatch/> <ActionMatch/>, or <EnvironmentMatch/>
class XACMLTargetMatch {
public:
  XACMLTarget(Arc::XMLNode& node, EvaluatorContext* ctx);
  virtual ~XACMLTargetMatch();
  virtual MatchResult match(EvaluationCtx* ctx);

private:
  AttributeFactory* attrfactory;
  FnFactoty* fnfactory;
  Arc::XMLNode matchnode;
  std::string matchId;

  Function* function;
  AttributeDesignator* designator;
  AttributeSelector* selector;
};

//node in higher level of above one, <Subject/> <Resource/> <Action/>, or <Environment/>
class XACMLTargetMatchGroup {
public:
  XACMLTargetMatchGroup(Arc::XMLNode& node, EvaluatorContext* ctx);
  virtual ~XACMLTargetMatchGroup();
  virtual MatchResult match(EvaluationCtx* ctx);

private:
  Arc::XMLNode matchgrpnode;
  std::list<XACMLTargetMatch*> matches;
};

//node in higher level of above one, <Subjects/> <Resources/> <Actions/>, or <Environments/>
class XACMLTargetSection {
public:
  XACMLTargetSection(Arc::XMLNode& node, EvaluatorContext* ctx);
  virtual ~XACMLTargetSection();
  virtual MatchResult match(EvaluationCtx* ctx);

private:
  Arc::XMLNode sectionnode;
  std::list<XACMLTargetMatchGroup*> groups;
};

///XACMLTarget class to parse and operate XACML specific <Target> node
//node in higher level of above one, <Target/>
class XACMLTarget {
public:
  /**Constructor - */
  XACMLTarget(Arc::XMLNode& node, EvaluatorContext* ctx);  
  virtual ~XACMLTarget();  
  virtual MatchResult match(EvaluationCtx* ctx);

private:
  Arc::XMLNode targetnode;
  std::list<XACMLTargetMatchSection*> sections;
};

} // namespace ArcSec

#endif /* __ARC_SEC_XACMLTARGET_H__ */

