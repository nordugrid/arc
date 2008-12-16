#include <arc/security/ArcPDP/policy/Policy.h>
#include <arc/security/ArcPDP/EvaluatorLoader.h>
#include <arc/Utils.h>

#include "InfoFilter.h"

namespace Arc {

class InfoPolicy {
 public:
  XMLNode xml;
  ArcSec::Policy* obj;
  ArcSec::Result res;
  InfoPolicy(void):xml(),obj(NULL),res(ArcSec::DECISION_DENY) { };
  InfoPolicy(XMLNode node):xml(node),obj(NULL),res(ArcSec::DECISION_DENY) { };
  ~InfoPolicy(void) { };
  void Destroy(void) { if(obj) delete obj; };
  InfoPolicy& operator=(ArcSec::Policy* policy) { obj=policy; };
  InfoPolicy& operator=(ArcSec::Result result) { res=result; };
};

static bool FilterNode(MessageAuth& id,XMLNode node,std::list< std::pair<XMLNode,XMLNode> >& policies,std::map<std::string,InfoPolicy>& ipolicies) {
  // Check if node has external policy







  // Check for internal policy
  // 1. Pick policy definitions
  XMLNode def = node["InfoFilterDefinition"];
  for(;(bool)def;++def) {
    // Create policy and store it in map
    // TODO: policies without identifier
    std::string pid = node.Attribute("id");
    ipolicies[pid]=InfoPolicy(def);
  };
  // 2. Check for tag
  std::string tag = node.Attribute("InfoFilterTag");
  if(!tag.empty()) {
    InfoPolicy& policy = ipolicies[tag];
    if(!policy.xml) { // No such policy defined
      return false;
    };
    if(!policy.obj) {
      // Parse internal policy
      ArcSec::EvaluatorLoader eloader;
      policy.obj=eloader.getPolicy(ArcSec::Source(policy.xml));
      if(!policy.obj) { // Failed to parse policy
        return false;
      };
      // Find proper evaluator
      AutoPointer<ArcSec::Evaluator> eval(eloader.getEvaluator(policy.obj));
      if(!eval) { // Failed to find proper evaluator
        return false;
      };
      // Generate request from identity of requestor
      XMLNode req;
      // TODO: do it only once
      if(!id.Export(eval->getName(),req)) { // Failed to generate request
        return false;
      };
      // Evaluate internal policy
      AutoPointer<ArcSec::Response> resp(eval->evaluate(ArcSec::Source(req),policy.obj));
      if(!resp) { // Failed to evaluate policy
        return false;
      };
      ArcSec::ResponseList& rlist = resp->getResponseItems();
      // Most probably there will be only one item. So far
      // using hardcoded prorities for response results.
      int res_deny = 0;
      int res_permit = 0;
      int res_notapplicable = 0;
      int res_indeteminate = 0;
      for(int n = 0;n<rlist.size();++n) {
        ArcSec::ResponseItem* ritem = rlist.getItem(n);
        if(ritem) {
          switch(ritem->res) {
            case ArcSec::DECISION_PERMIT: ++res_permit; break;
            case ArcSec::DECISION_DENY: ++res_deny; break;
            case ArcSec::DECISION_INDETERMINATE: ++res_indeteminate; break;
            case ArcSec::DECISION_NOT_APPLICABLE: ++res_notapplicable; break;
            default: ++res_deny; break; // Safe
          };
        };
      };
      // Store evaluation result
      if(res_deny) { policy.res=ArcSec::DECISION_DENY; }
      else if(res_permit) { policy.res=ArcSec::DECISION_PERMIT; }
      else if(res_notapplicable) { policy.res=ArcSec::DECISION_NOT_APPLICABLE; }
      else if(res_indeteminate) { policy.res=ArcSec::DECISION_INDETERMINATE; };
    };
    if(policy.res != ArcSec::DECISION_PERMIT) {
      node.Destroy();
      // TODO: take care of collected nodes for external policies
      return true;
    };
  };
  // Process children nodes
  XMLNode cnode = node.Child();
  for(;(bool)cnode;++cnode) {
    if(!FilterNode(id,cnode,policies,ipolicies)) return false;
  };
  return true;
}

InfoFilter::InfoFilter(MessageAuth& id):id_(id) {
}

bool InfoFilter::Filter(XMLNode doc) {
  std::list< std::pair<std::string,XMLNode> > policies;
  Arc::NS ns;
  return Filter(doc,policies,ns);
}

bool InfoFilter::Filter(XMLNode doc,const std::list< std::pair<std::string,XMLNode> >& policies,const Arc::NS& ns) {
  std::map<std::string,InfoPolicy> ipolicies_; // pre-parsed policies
  // Create list of nodes associated to external policies
  std::list< std::pair<XMLNode,XMLNode> > policies_;
  for(std::list< std::pair<std::string,XMLNode> >::const_iterator p = policies.begin();
                     p != policies.end();++p) {
    XMLNodeList nodes = doc.XPathLookup(p->first,ns);
    for(XMLNodeList::iterator n = nodes.begin();n != nodes.end();++n) {
      policies_.push_back(std::pair<XMLNode,XMLNode>(*n,p->second));
    };
  };
  // Go through nodes and check policies
  bool r = FilterNode(id_,doc,policies_,ipolicies_); 
  for(std::map<std::string,InfoPolicy>::iterator i = ipolicies_.begin();
                     i != ipolicies_.end();++i) {
    i->second.Destroy();
  };
  return r;
}


} // namespace Arc

