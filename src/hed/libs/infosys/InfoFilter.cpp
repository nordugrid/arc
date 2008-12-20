#include <arc/security/ArcPDP/policy/Policy.h>
#include <arc/security/ArcPDP/EvaluatorLoader.h>
#include <arc/Utils.h>

#include "InfoFilter.h"

namespace Arc {

class InfoPolicy {
 public:
  XMLNode xml;
  bool done;
  ArcSec::Result res;
  InfoPolicy(void):xml(),done(false),res(ArcSec::DECISION_DENY) { };
  InfoPolicy(XMLNode node):xml(node),done(false),res(ArcSec::DECISION_DENY) { };
  ~InfoPolicy(void) { };
  bool Evaluate(MessageAuth& id);
};


bool InfoPolicy::Evaluate(MessageAuth& id) {
  if(done) return true;
  // Parse internal policy
  ArcSec::EvaluatorLoader eloader;
  AutoPointer<ArcSec::Policy> policy(eloader.getPolicy(ArcSec::Source(xml)));
  if(!policy) { // Failed to parse policy
    return false;
  };
  // Find proper evaluator
  AutoPointer<ArcSec::Evaluator> eval(eloader.getEvaluator(policy));
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
  AutoPointer<ArcSec::Response> resp(eval->evaluate(ArcSec::Source(req),policy));
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
  if(res_deny) { res=ArcSec::DECISION_DENY; }
  else if(res_permit) { res=ArcSec::DECISION_PERMIT; }
  else if(res_notapplicable) { res=ArcSec::DECISION_NOT_APPLICABLE; }
  else if(res_indeteminate) { res=ArcSec::DECISION_INDETERMINATE; };
  return true;
}


static void RemovePolicies(std::list< std::pair<XMLNode,std::list<InfoPolicy>::iterator> >& policies,XMLNode node) {
  // Remove nodes associated with external policies.
  // This is not most effective way to handle this problem, but
  // currently I could not find anything better.
  for(std::list< std::pair<XMLNode,std::list<InfoPolicy>::iterator> >::iterator p = policies.begin();
                                         p != policies.end();) {
    if(node == p->first) {
      p=policies.erase(p);
    } else {
      ++p;
    };
  };
  // Process children nodes
  XMLNode cnode = node.Child();
  for(;(bool)cnode;++cnode) RemovePolicies(policies,cnode);
}

static void RemoveEmbeddedPolicies(XMLNode node) {
  // Remove all children policies
  while(true) {
    XMLNode def = node["InfoFilterDefinition"];
    if(!def) break;
    def.Destroy();
  };

  // Remove tag
  XMLNode tag = node.Attribute("InfoFilterTag");
  tag.Destroy();

  // Process children nodes
  XMLNode cnode = node.Child();
  for(;(bool)cnode;++cnode) RemoveEmbeddedPolicies(cnode);
  return;
}

static bool FilterNode(MessageAuth& id,XMLNode node,std::list< std::pair<XMLNode,std::list<InfoPolicy>::iterator> >& policies,std::list<InfoPolicy>& epolicies,std::map<std::string,InfoPolicy>& ipolicies) {
  // Check if node has external policy
  for(std::list< std::pair<XMLNode,std::list<InfoPolicy>::iterator> >::iterator p = policies.begin();
                                         p != policies.end();) {
    if(node == p->first) {
      // Node has assigned policy - evaluate it
      if(!p->second->Evaluate(id)) { // Policy evaluation failed
        return false;
      };
      if(p->second->res != ArcSec::DECISION_PERMIT) {
        RemovePolicies(policies,node);
        node.Destroy();
        break;
      };
    };
    ++p;
  };
  if((bool)node) {
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
    // TODO: policies without ids and tags
    std::string tag = node.Attribute("InfoFilterTag");
    if(!tag.empty()) {
      InfoPolicy& policy = ipolicies[tag];
      if(!policy.xml) { // No such policy defined
        return false;
      };
      if(!policy.Evaluate(id)) { // Failed to evaluate policy
        return false;
      };
      if(policy.res != ArcSec::DECISION_PERMIT) {
        RemovePolicies(policies,node);
        node.Destroy();
        return true;
      };
    };
  };
  if((bool)node) {
    // Process children nodes
    XMLNode cnode = node.Child();
    for(;(bool)cnode;++cnode) {
      if(!FilterNode(id,cnode,policies,epolicies,ipolicies)) return false;
    };
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
  std::map<std::string,InfoPolicy> ipolicies_; // internal policies associated to their ids
  std::list<InfoPolicy> epolicies_; // external policies
  std::list< std::pair<XMLNode,std::list<InfoPolicy>::iterator> > policies_; // nodes associated to external policies
  for(std::list< std::pair<std::string,XMLNode> >::const_iterator p = policies.begin();
                     p != policies.end();++p) {
    XMLNodeList nodes = doc.XPathLookup(p->first,ns);
    if(nodes.size() > 0) {
      std::list<InfoPolicy>::iterator ep = epolicies_.insert(epolicies_.end(),InfoPolicy(p->second));
      for(XMLNodeList::iterator n = nodes.begin();n != nodes.end();++n) {
        policies_.push_back(std::pair<XMLNode,std::list<InfoPolicy>::iterator>(*n,ep));
      };
    };
  };
  // Go through nodes and check policies
  bool r = FilterNode(id_,doc,policies_,epolicies_,ipolicies_); 
  if(!r) return false;
  // Remove policies embedded into document
  RemoveEmbeddedPolicies(doc);
  return true;
}


} // namespace Arc

