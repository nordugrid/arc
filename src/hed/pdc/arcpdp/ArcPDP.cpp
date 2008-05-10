#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>
#include <arc/loader/PDPLoader.h>
//#include <arc/loader/ClassLoader.h>
#include <arc/XMLNode.h>
#include <arc/Thread.h>
#include <arc/ArcConfig.h>
#include <arc/ArcLocation.h>
#include <arc/Logger.h>

#include <arc/security/ArcPDP/Response.h>

#include <arc/security/ArcPDP/attr/AttributeValue.h>

#include "ArcPDP.h"

Arc::Logger ArcSec::ArcPDP::logger(ArcSec::PDP::logger,"ArcPDP");

/*
static ArcSec::PDP* get_pdp(Arc::Config *cfg,Arc::ChainContext *ctx) {
    return new ArcSec::ArcPDP(cfg);
}

pdp_descriptors ARC_PDP_LOADER = {
    { "arc.pdp", 0, &get_pdp},
    { NULL, 0, NULL }
};
*/

using namespace Arc;

namespace ArcSec {

PDP* ArcPDP::get_arc_pdp(Config *cfg,ChainContext*) {
    return new ArcPDP(cfg);
}

// This class is used to store Evaluator per connection
class ArcPDPContext:public Arc::MessageContextElement {
 friend class ArcPDP;
 private:
  Evaluator* eval;
 public:
  ArcPDPContext(Evaluator* e);
  ArcPDPContext(void);
  virtual ~ArcPDPContext(void);
};

ArcPDPContext::~ArcPDPContext(void) {
  if(eval) delete eval;
}

ArcPDPContext::ArcPDPContext(Evaluator* e):eval(e) {
}

ArcPDPContext::ArcPDPContext(void):eval(NULL) {
  XMLNode pdp_cfg_nd("\
    <ArcConfig\
     xmlns=\"http://www.nordugrid.org/schemas/ArcConfig/2007\"\
     xmlns:pdp=\"http://www.nordugrid.org/schemas/pdp/Config\">\
     <ModuleManager>\
        <Path></Path>\
     </ModuleManager>\
     <Plugins Name='arcpdc'>\
          <Plugin Name='__arc_attrfactory_modules__'>attrfactory</Plugin>\
          <Plugin Name='__arc_fnfactory_modules__'>fnfactory</Plugin>\
          <Plugin Name='__arc_algfactory_modules__'>algfactory</Plugin>\
          <Plugin Name='__arc_evaluator_modules__'>evaluator</Plugin>\
          <Plugin Name='__arc_request_modules__'>request</Plugin>\
     </Plugins>\
     <pdp:PDPConfig>\
          <pdp:AttributeFactory name='attr.factory' />\
          <pdp:CombingAlgorithmFactory name='alg.factory' />\
          <pdp:FunctionFactory name='fn.factory' />\
          <pdp:Evaluator name='arc.evaluator' />\
          <pdp:Request name='arc.request' />\
     </pdp:PDPConfig>\
    </ArcConfig>");

  //Get the lib path from environment, and put it into the configuration xml node
  std::list<std::string> plugins = ArcLocation::GetPlugins();
  for(std::list<std::string>::iterator p = plugins.begin();p!=plugins.end();++p)
    pdp_cfg_nd["ModuleManager"].NewChild("Path")=*p;

  Config modulecfg(pdp_cfg_nd);

  Arc::ClassLoader* classloader = ClassLoader::getClassLoader(&modulecfg);
  std::string evaluator = "arc.evaluator";

  //Dynamically load Evaluator object according to configure information
  eval = dynamic_cast<Evaluator*>(classloader->Instance(evaluator, (void**)(void*)&pdp_cfg_nd));
}

ArcPDP::ArcPDP(Config* cfg):PDP(cfg) /*, eval(NULL)*/ {
  XMLNode pdp_node(*cfg);

  XMLNode filter = (*cfg)["Filter"];
  if((bool)filter) {
    XMLNode select_attr = filter["Select"];
    XMLNode reject_attr = filter["Reject"];
    for(;(bool)select_attr;++select_attr) select_attrs.push_back((std::string)select_attr);
    for(;(bool)reject_attr;++reject_attr) reject_attrs.push_back((std::string)reject_attr);
  };
  XMLNode policy_location = (*cfg)["PolicyStore"];
  for(;(bool)policy_location;++policy_location) policy_locations.push_back((std::string)policy_location);
}

bool ArcPDP::isPermitted(Message *msg){
  //Compose Request based on the information inside message, the Request will be like below:
  /*
  <Request xmlns="http://www.nordugrid.org/schemas/request-arc">
    <RequestItem>
        <Subject>
          <Attribute AttributeId="123" Type="string">123.45.67.89</Attribute>
          <Attribute AttributeId="xyz" Type="string">/O=NorduGrid/OU=UIO/CN=test</Attribute>
        </Subject>
        <Action AttributeId="ijk" Type="string">GET</Action>
    </RequestItem>
  </Request>
  */
  Evaluator* eval = NULL;

  PDPConfigContext *config = NULL;
  try {
    Arc::MessageContextElement* context = (*(msg->Context()))[id_];;
    if(context) { 
      config = dynamic_cast<PDPConfigContext*>(context);
    }
  } catch(std::exception& e) { };
  if(config == NULL) {
    logger.msg(INFO,"Although there is pdp configuration for this component, no pdp context has been generated for it"); 
  };

  std::string ctxid = "arcsec.arcpdp."+id_;
  try {
    // Using ID of PDP here to allow for multiple ArcPDP in a chain.
    // If PDPs with same IDs are present user must understand that 
    // they will be merged.
    Arc::MessageContextElement* mctx = (*(msg->Context()))[ctxid];
    if(mctx) {
      ArcPDPContext* pdpctx = dynamic_cast<ArcPDPContext*>(mctx);
      if(pdpctx) {
        eval=pdpctx->eval;
      };
    };
  } catch(std::exception& e) { };
  if(!eval) {
    ArcPDPContext* pdpctx = new ArcPDPContext();
    if(pdpctx) {
      eval=pdpctx->eval;
      if(eval) {
        std::list<std::string> policylocation = (config!=NULL)?config->GetPolicyLocation():policy_locations;
        for(std::list<std::string>::iterator it = policylocation.begin(); it!= policylocation.end(); it++) {
          eval->addPolicy(*it);
        }
        msg->Context()->Add(ctxid,pdpctx);
      } else {
        delete pdpctx;
      }
    }
    if(!eval) logger.msg(ERROR, "Can not dynamically produce Evaluator");
  }
  if(!eval) {
    logger.msg(ERROR,"Evaluator for ArcPDP was not loaded"); 
    return false;
  };

  /*

  NS ns;
  ns["ra"]="http://www.nordugrid.org/schemas/request-arc";
  XMLNode requestxml(ns,"ra:Request");

  for(int i = 0; i<config->RequestItemSize(); i++) {
    XMLNode requestitem = requestxml.NewChild("ra:RequestItem");
    XMLNode sub = requestitem.NewChild("ra:Subject");

    ArcSec::AuthzRequest& request = config->GetRequestItem(i);      

    std::list<ArcSec::AuthzRequestSection>& section = request.subject;
    for(std::list<ArcSec::AuthzRequestSection>::iterator it = section.begin(); it != section.end(); it++) {
      XMLNode subattr = sub.NewChild("ra:Attribute");
      subattr = it->value;
      XMLNode subattrId = subattr.NewAttribute("ra:AttributeId");
      subattrId = it->id;
      XMLNode subattrType = subattr.NewAttribute("ra:Type");
      subattrType = it->type;
      XMLNode subattrIssuer = subattr.NewAttribute("ra:Issuer");
      subattrIssuer = it->issuer;
    }

    XMLNode res = requestitem.NewChild("ra:Resource");
    section = request.resource;
    for(std::list<ArcSec::AuthzRequestSection>::iterator it = section.begin(); it != section.end(); it++) {
      XMLNode resattr = res.NewChild("ra:Attribute");
      resattr = (*it).value;
      XMLNode resattrId = resattr.NewAttribute("ra:AttributeId");
      resattrId = (*it).id;
      XMLNode resattrType = resattr.NewAttribute("ra:Type");
      resattrType = (*it).type;
      XMLNode resattrIssuer = resattr.NewAttribute("ra:Issuer");
      resattrIssuer = (*it).issuer;
    }

    XMLNode act = requestitem.NewChild("ra:Action");
    section = request.action;
    for(std::list<ArcSec::AuthzRequestSection>::iterator it = section.begin(); it != section.end(); it++) {
      XMLNode actattr = act.NewChild("ra:Attribute");
      actattr = (*it).value;
      XMLNode actattrId = actattr.NewAttribute("ra:AttributeId");
      actattrId = (*it).id;
      XMLNode actattrType = actattr.NewAttribute("ra:Type");
      actattrType = (*it).type;
      XMLNode actattrIssuer = actattr.NewAttribute("ra:Issuer");
      actattrIssuer = (*it).issuer;
    }

    XMLNode ctx = requestitem.NewChild("ra:Context");
    section = request.context;
    for(std::list<ArcSec::AuthzRequestSection>::iterator it = section.begin(); it != section.end(); it++) {
      XMLNode ctxattr = ctx.NewChild("ra:Attribute");
      ctxattr = (*it).value;
      XMLNode ctxattrId = ctxattr.NewAttribute("ra:AttributeId");
      ctxattr = (*it).id;
      XMLNode ctxattrType = ctxattr.NewAttribute("ra:Type");
      ctxattr = (*it).type;
      XMLNode ctxattrIssuer = ctxattr.NewAttribute("ra:Issuer");
      ctxattr = (*it).issuer;
    }
  }
  */

  MessageAuth* mauth = msg->Auth()->Filter(select_attrs,reject_attrs);
  if(!mauth) {
    logger.msg(ERROR,"Missing security object in message");
    return false;
  };
  NS ns;
  XMLNode requestxml(ns,"");
  if(!mauth->Export(SecAttr::ARCAuth,requestxml)) {
    delete mauth;
    logger.msg(ERROR,"Failed to convert security information to ARC request");
    return false;
  };
  delete mauth;
  {
    std::string s;
    requestxml.GetXML(s);
    logger.msg(VERBOSE,"ARC Auth. request: %s",s);
  };
  if(requestxml.Size() <= 0) {
    logger.msg(ERROR,"No requested security information was collected");
    return false;
  };

  //Call the evaluation functionality inside Evaluator
  Response *resp = eval->evaluate(requestxml);
  logger.msg(INFO, "There are %d requests, which satisfy at least one policy", (resp->getResponseItems()).size());
  ResponseList rlist = resp->getResponseItems();
  int size = rlist.size();
  for(int i = 0; i < size; i++) {
    ResponseItem* item = rlist[i];
    RequestTuple* tp = item->reqtp;
    Subject::iterator it;
    Subject subject = tp->sub;
    for (it = subject.begin(); it!= subject.end(); it++){
      AttributeValue *attrval;
      RequestAttribute *attr;
      attr = dynamic_cast<RequestAttribute*>(*it);
      if(attr){
        attrval = (*it)->getAttributeValue();
        if(attrval) logger.msg(INFO, "%s", attrval->encode());
      }
    }
  } 

  if(!(rlist.empty())){
    logger.msg(INFO, "Authorized from arc.pdp!!!");
    if(resp)
      delete resp;
    return true;
  }
  else {
    logger.msg(ERROR, "UnAuthorized from arc.pdp!!!");
    if(resp)
      delete resp;
    return false;
  }
}

ArcPDP::~ArcPDP(){
  //if(eval)
  //  delete eval;
  //eval = NULL;
}

} // namespace ArcSec

