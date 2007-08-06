#include "ArcRequestItem.h"

ArcRequestItem::ArcRequestItem(const Arc::XMLNode& node){
  ArcAttributeFactory * attrfactory;

  Arc::NS nsList;
  nsList.insert(std::pair<std::string, std::string>("request","http://www.nordugrid.org/ws/schemas/request-arc"));
  std::list<Arc::XMLNode> list;
  std::list<Arc::XMLNode>::iterator it;
  std::list<Arc::XMLNode> attrlist;
  std::list<Arc::XMLNode>::iterator attrit;

  list = node.XPathLookup("//request:Subject", nsList);
  for ( it=list.begin() ; it != list.end(); it++ ){
    XMLNode nd=*it;
    attrlist = nd.XPathLookup("//request:Attribute", nsList)   
    
    /**if is like this: 
     <Subject AttributeId="urn:arc:subject:dn" Type="X500DN">/O=NorduGrid/OU=UIO/CN=test</Subject>  */
    if(attrlist.empty()){
      Subject sub;
      sub.push_back(new Arc::RequestAttribute(nd));
      subjects.push_back(sub);  
    }
    /**else like this:
        <Subject>
          <Attribute AttributeId="urn:arc:subject:voms-attribute" Type="xsd:string">administrator</Attribute>
          <Attribute AttributeId="urn:arc:subject:voms-attribute" Type="X500DN">/O=NorduGrid/OU=UIO/CN=admin</Attribute>
        </Subject> */
    else{
      Subject sub;
      for (attrit=attrlist.begin() ; attrit != attrlist.end(); attrit++ ){
        sub.push_back(new Arc::RequestAttribute(*attrit, attrfactory));
      }
      subjects.push_back(sub);
    }
  }
  
  list = node.XPathLookup("//request:Resource", nsList);
  for ( it=list.begin() ; it != list.end(); it++ ){
    XMLNode nd=*it;
    attrlist = nd.XPathLookup("//request:Attribute", nsList)

    if(attrlist.empty()){
      Resource res;
      res.push_back(new Arc::RequestAttribute(nd, attrfactory));
      resources.push_back(res);
    }
    else{
      Resource sub;
      for (attrit=attrlist.begin() ; attrit != attrlist.end(); attrit++ ){
        res.push_back(new Arc::RequestAttribute(*attrit, attrfactory));
      }
      resources.push_back(res);
    }
  }

  list = node.XPathLookup("//request:Action", nsList);
  for ( it=list.begin() ; it != list.end(); it++ ){
    XMLNode nd=*it;
    attrlist = nd.XPathLookup("//request:Attribute", nsList)

    if(attrlist.empty()){
      Action act;
      act.push_back(new Arc::RequestAttribute(nd, attrfactory));
      resources.push_back(act);
    }
    else{
      Action act;
      for (attrit=attrlist.begin() ; attrit != attrlist.end(); attrit++ ){
        act.push_back(new Arc::RequestAttribute(*attrit, attrfactory));
      }
      actions.push_back(act);
    }
  }

  list = node.XPathLookup("//request:Environment", nsList);
  for ( it=list.begin() ; it != list.end(); it++ ){
    XMLNode nd=*it;
    attrlist = nd.XPathLookup("//request:Attribute", nsList)

    if(attrlist.empty()){
      Environment env;
      env.push_back(new Arc::RequestAttribute(nd, attrfactory));
      environments.push_back(env);
    }
    else{
      Environment env;
      for (attrit=attrlist.begin() ; attrit != attrlist.end(); attrit++ ){
        env.push_back(new Arc::RequestAttribute(*attrit, attrfactory));
      }
      environments.push_back(env);
    }
  }
}

ArcRequestItem:: ~RequestItem(void){
  while(!subjects.empty()){
    Subject sub = subjects.back();
    while(!sub.empty()){
      delete sub.back();
      sub.pop_back();
    }
    subjects.pop_back();
  }

  while(!resources.empty()){
    Resource res = resources.back();
    while(!res.empty()){
      delete res.back();
      res.pop_back();
    }
    resources.pop_back();
  }

  while(!actions.empty()){
    Action act = actions.back();
    while(!act.empty()){
      delete act.back();
      act.pop_back();
    }
    actions.pop_back();
  }

  while(!environments.empty()){
    Environment env = subjects.back();
    while(!env.empty()){
      delete env.back();
      env.pop_back();
    }
    environments.pop_back();
  }
}

SubList ArcRequestItem::getSubjects () const{
  return subjects;
}

void ArcRequestItem::setSubjects (const SubList& sl){
  subjects = sl;
}

ResList ArcRequestItem::getResources () const{
  return resources;
}

void ArcRequestItem::setResources (const ResList& rl){
  resources = rl;
}

ActList ArcRequestItem::getActions () const {
  return actions;
}

void ArcRequestItem::setAction (const ActList& al){
  actions = al;
}

EnvList ArcRequestItem::getEnvironments () const{
  return environments;
}

void ArcRequestItem::setEnvironmets (const EnvList& ctx){
  environments = ctx;
}

