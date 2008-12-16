#include <arc/security/ArcPDP/policy/Policy.h>

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
  InfoPolicy& operator =(ArcSec:Policy* policy) { obj=policy; };
  InfoPolicy& operator =(ArcSec:Result result) { res=result; };
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

      // Evaluate internal policy

      // Store evaluation result

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
  std::list< std::pair<std::string,XMLNode> policies;
  Arc::NS ns;
  return Filter(doc,policies,ns);
}

bool Filter(XMLNode doc,const std::list< std::pair<std::string,XMLNode> >& policies,const Arc::NS& ns) {
  std::map<std::string,InfoPolicy> ipolicies_; // pre-parsed policies
  // Create list of nodes associated to external policies
  std::list< std::pair<XMLNode,XMLNode> > policies_;
  for(std::list< std::pair<std::string,XMLNode> >::const_iterator p = policies.begin();
                     p != policies.end();++p) {
    XMLNodeList nodes = doc.XPathLookup(p->first,ns);
    for(XMLNodeList::iterator n = nodes.begin();n != nodes.end();++n) {
      policies.push_back(std::pair<XMLNode,XMLNode>(*n,p->second));
    };
  };
  // Go through nodes and check policies
  bool r = FilterNode(doc,policies_,ipolicies_); 
  for(std::map<std::string,InfoPolicy>::iterator i = ipolicies_.begin();
                     i != ipolicies_.end();++i) {
    i->second.Destroy();
  };
  return r;
}


} // namespace Arc

