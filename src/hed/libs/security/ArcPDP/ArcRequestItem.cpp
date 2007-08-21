#include "ArcRequestItem.h"

ArcRequestItem::ArcRequestItem(const Arc::XMLNode& node){
  //TODO
  ArcAttributeFactory * attrfactory = new ArcAttributeFactory();
  XMLNode nd;

  for ( int i=0;; i++ ){
    nd = node["Subject"][i];
    if(!nd) break;
    
    /**if is like this: 
     <Subject AttributeId="urn:arc:subject:dn" Type="X500DN">/O=NorduGrid/OU=UIO/CN=test</Subject>  */
    if(nd.AttributesSize()){
      Subject sub;
      sub.push_back(new Arc::RequestAttribute(nd, attrfactory));
      subjects.push_back(sub);  
    }
    /**else like this:
        <Subject>
          <Attribute AttributeId="urn:arc:subject:voms-attribute" Type="xsd:string">administrator</Attribute>
          <Attribute AttributeId="urn:arc:subject:voms-attribute" Type="X500DN">/O=NorduGrid/OU=UIO/CN=admin</Attribute>
        </Subject> */
    else{
      Subject sub;
      for(int j=0;;j++){
        XMLNode tnd = nd.Child(j);
        if(!tnd) break;
        sub.push_back(new Arc::RequestAttribute(tnd, attrfactory));
      }
      subjects.push_back(sub);
    }
  }

  for ( int i=0;; i++ ){
    nd = node["Resource"][i];
    if(!nd) break;

    if(nd.AttributesSize()){
      Resource res;
      res.push_back(new Arc::RequestAttribute(nd, attrfactory));
      resources.push_back(res);
    }
    else{
      Resource res;
      for(int j=0;;j++){
        XMLNode tnd = nd.Child(j);
        if(tnd) break;
        res.push_back(new Arc::RequestAttribute(tnd, attrfactory));
      }
      resources.push_back(res);
    }
  }

  for ( int i=0;; i++ ){
    nd = node["Action"][i];
    if(!nd) break;

    if(nd.AttributesSize()){
      Action act;
      act.push_back(new Arc::RequestAttribute(nd, attrfactory));
      actions.push_back(act);
    }
    else{
      Action act;
      for(int j=0;;j++){
        XMLNode tnd = nd.Child(j);
        if(tnd) break;
        act.push_back(new Arc::RequestAttribute(tnd, attrfactory));
      }
      actions.push_back(act);
    }
  }

  for ( int i=0;; i++ ){
    nd = node["Environment"][i];
    if(!nd) break;

    if(nd.AttributesSize()){
      Environment env;
      env.push_back(new Arc::RequestAttribute(nd, attrfactory));
      environments.push_back(env);
    }
    else{
      Environment env;
      for(int j=0;;j++){
        XMLNode tnd = nd.Child(j);
        if(tnd) break;
        env.push_back(new Arc::RequestAttribute(tnd, attrfactory));
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

