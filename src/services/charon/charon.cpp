#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#ifndef WIN32
#include <pwd.h>
#endif

#include <arc/message/PayloadSOAP.h>
#include <arc/message/PayloadRaw.h>
#include <arc/message/PayloadStream.h>
#include <arc/ws-addressing/WSA.h>
#include <arc/security/ArcPDP/EvaluatorLoader.h>
#include <arc/security/ArcPDP/Source.h>
#include <arc/Thread.h>
#include <arc/StringConv.h>
#include <arc/GUID.h>

#include "charon.h"

namespace ArcSec {

#define SAML_NAMESPACE "urn:oasis:names:tc:SAML:2.0:assertion"
#define SAMLP_NAMESPACE "urn:oasis:names:tc:SAML:2.0:protocol"
#define XACML_SAMLP_NAMESPACE "urn:oasis:names:tc:xacml:2.0:saml:protocol:schema:os"
#define XACML_SAML_NAMESPACE "urn:oasis:names:tc:xacml:2.0:saml:assertion:schema:os"
#define XACML_CONTEXT_NAMESPACE "urn:oasis:names:tc:xacml:2.0:context:schema:os"

static Arc::LogStream logcerr(std::cerr);

static Arc::Plugin* get_service(Arc::PluginArgument* arg) {
    Arc::ServicePluginArgument* srvarg =
            arg?dynamic_cast<Arc::ServicePluginArgument*>(arg):NULL;
    if(!srvarg) return NULL;
    return new Charon((Arc::Config*)(*srvarg));
}

Arc::MCC_Status Charon::make_soap_fault(Arc::Message& outmsg, const std::string& msg) {
  Arc::PayloadSOAP* outpayload = new Arc::PayloadSOAP(ns_,true);
  Arc::SOAPFault* fault = outpayload?outpayload->Fault():NULL;
  if(fault) {
    fault->Code(Arc::SOAPFault::Sender);
    if(msg.empty()) {
      fault->Reason("Failed processing request");
    } else {
      fault->Reason(msg);
    };
  };
  outmsg.Payload(outpayload);
  return Arc::MCC_Status(Arc::STATUS_OK);
}

Arc::MCC_Status Charon::process(Arc::Message& inmsg,Arc::Message& outmsg) {

  std::string method = inmsg.Attributes()->get("HTTP:METHOD");

  // Identify which of served endpoints request is for.
  // Charon can only accept POST method
  if(method == "POST") {
    logger.msg(Arc::VERBOSE, "process: POST");
    // Both input and output are supposed to be SOAP
    // Extracting payload
    Arc::PayloadSOAP* inpayload = NULL;
    try {
      inpayload = dynamic_cast<Arc::PayloadSOAP*>(inmsg.Payload());
    } catch(std::exception& e) { };
    if(!inpayload) {
      logger.msg(Arc::ERROR, "input is not SOAP");
      return make_soap_fault(outmsg);
    };
    // Analyzing request
    Arc::XMLNode request = (*inpayload)["GetPolicyDecisionRequest"];
    if(!request) {
      request = (*inpayload)["XACMLAuthzDecisionQuery"];
      if(!request) {
        logger.msg(Arc::ERROR, "soap body does not include any request node");
        return make_soap_fault(outmsg);
      };
    };
    {
      std::string req_xml;
      request.GetXML(req_xml);
      logger.msg(Arc::VERBOSE, "Request: %s",req_xml);
    };    

    Arc::XMLNode arc_requestnd;
    bool use_saml = false;
    std::string request_name = request.Name();
    if(request_name.find("GetPolicyDecisionRequest") != std::string::npos)
      arc_requestnd = request["Request"];
    else if(request_name.find("XACMLAuthzDecisionQuery") != std::string::npos) {
      arc_requestnd = request["Request"];
      use_saml = true;
    }
    if(!arc_requestnd) {
      logger.msg(Arc::ERROR, "request node is empty");
      return make_soap_fault(outmsg);
    };

    if(!eval) {
      logger.msg(Arc::ERROR, "Evaluator is not initialized");
      return make_soap_fault(outmsg,"Internal policy processing error");
    };

    //Call the functionality of policy engine
    //Here the decision algorithm is the same as that in ArcPDP.cpp,
    //so this algorithm implicitly applies to policy with Arc format;
    //for xacml policy (the "rlist" will only contains one item), this 
    //decision algorithm also applies.
    Response *resp = NULL;
    // Evaluator stores results inside, hence can't be used multi-threaded
    // TODO: fix multi-threading for Evaluator
    Glib::Mutex::Lock eval_lock(lock_);
    if(policies_modified()) {
      logger.msg(Arc::INFO, "Policy(ies) modified - reloading evaluator");
      load_policies();
    }
    resp = eval->evaluate(Source(arc_requestnd));
    ResponseList rlist = resp->getResponseItems();
    int size = rlist.size();
    int i;

    bool atleast_onedeny = false;
    bool atleast_onepermit = false;

    for(i = 0; i < size; i++){
      ResponseItem* item = rlist[i];
      RequestTuple* tp = item->reqtp;

      if(item->res == DECISION_DENY)
        atleast_onedeny = true;
      if(item->res == DECISION_PERMIT)
        atleast_onepermit = true;
     
      if(tp) {
        Subject::iterator it;
        Subject subject = tp->sub;
        for (it = subject.begin(); it!= subject.end(); it++){
          AttributeValue *attrval;
          RequestAttribute *attr;
          attr = dynamic_cast<RequestAttribute*>(*it);
          if(attr){
            attrval = (*it)->getAttributeValue();
            if(attrval) logger.msg(Arc::INFO, "%s", (attrval->encode()));
          }
        }
      }
    }

    bool result = false;
    if(atleast_onedeny) result = false;
    else if(!atleast_onedeny && atleast_onepermit) result = true;
    else if(!atleast_onedeny && !atleast_onepermit) result = false;

    if(result) logger.msg(Arc::INFO, "Authorized from Charon service");
    else logger.msg(Arc::ERROR, "Not authorized from Charon service; Some of the RequestItem does not satisfy Policy");

    // Put those request items which satisfy the policies into the 
    // response SOAP message (implicitly means the decision result: 
    // <ItemA, permit>, <ItemB, permit>, <ItemC, deny> (ItemC will 
    // not be in the response SOAP message).
    // The client of the pdpservice (normally a policy enforcement 
    // point, like job executor) is supposed to compose the policy
    // decision request to pdpservice by parsing the information 
    // from the request (aiming to the client itself), and permit 
    // or deny the request by using the information responded from 
    // pdpservice; Here pdpservice only gives some coarse decision 
    // to pdpservice invoker.
    if(!use_saml) {
      Arc::PayloadSOAP* outpayload = new Arc::PayloadSOAP(ns_);
      Arc::XMLNode response = outpayload->NewChild("pdp:GetPolicyDecisionResponse");
      Arc::XMLNode arc_responsend = response.NewChild("response:Response");
      Arc::XMLNode authz_res = arc_responsend.NewChild("response:AuthZResult");
      if(result) authz_res = "PERMIT";
      else authz_res = "DENY";

      for(i = 0; i<size; i++){
        ResponseItem* item = rlist[i];
        if(item->res == DECISION_PERMIT)
          arc_responsend.NewChild(Arc::XMLNode(item->reqxml));
      }
      outmsg.Payload(outpayload);

    } else {
      Arc::NS samlp_ns, saml_ns;
      samlp_ns["samlp"] = SAMLP_NAMESPACE;
      saml_ns["saml"] = SAML_NAMESPACE;
      saml_ns["xsi"] = "http://www.w3.org/2001/XMLSchema-instance";

      //<saml:Response/>
      Arc::XMLNode authz_response(samlp_ns, "samlp:Response");
      std::string local_dn = inmsg.Attributes()->get("TLS:LOCALDN");
      std::string issuer_name = Arc::convert_to_rdn(local_dn);
      authz_response.NewChild("samlp:Issuer") = issuer_name;

      std::string response_id = Arc::UUID();
      authz_response.NewAttribute("ID") = response_id;
      std::string responseto_id = (std::string)(request.Attribute("ID"));
      authz_response.NewAttribute("InResponseTo") = responseto_id;
      Arc::Time t;
      std::string current_time = t.str(Arc::UTCTime);
      authz_response.NewAttribute("IssueInstant") = current_time;
      authz_response.NewAttribute("Version") = std::string("2.0");

      Arc::XMLNode status = authz_response.NewChild("samlp:Status");
      Arc::XMLNode statuscode = status.NewChild("samlp:StatusCode");
      std::string statuscode_value = "urn:oasis:names:tc:SAML:2.0:status:Success";
      statuscode.NewAttribute("Value") = statuscode_value;

      //<saml:Assertion/>
      Arc::XMLNode assertion = authz_response.NewChild("saml:Assertion", saml_ns);
      assertion.NewAttribute("Version") = std::string("2.0");
      std::string assertion_id = Arc::UUID();
      assertion.NewAttribute("ID") = assertion_id;
      Arc::Time t1;
      std::string current_time1 = t1.str(Arc::UTCTime);
      assertion.NewAttribute("IssueInstant") = current_time1;
      assertion.NewChild("saml:Issuer") = issuer_name;

      //<saml:XACMLAuthzDecisionStatement/>
      Arc::NS xacml_saml_ns; xacml_saml_ns["xacml-saml"] = XACML_SAML_NAMESPACE;
      Arc::XMLNode authz_statement = assertion.NewChild("xacml-saml:XACMLAuthzDecisionStatement", xacml_saml_ns);
      //See "xacml-context" specification
      Arc::NS xacml_ns; xacml_ns["xacml-context"] = XACML_CONTEXT_NAMESPACE;
      Arc::XMLNode xacml_response = authz_statement.NewChild("xacml-context:Response", xacml_ns);
      Arc::XMLNode xacml_result = xacml_response.NewChild("xacml-context:Result");
      xacml_result.NewAttribute("ResourceId") = "resource-id"; //TODO
      if(result) xacml_result.NewChild("xacml-context:Decision") = "Permit";
      else xacml_result.NewChild("xacml-context:Decision") = "Deny"; //TODO: Indeterminate, NotApplicable
      //TODO: "xacml-context:Status"  "xacml:Obligations"


      Arc::NS soap_ns;
      Arc::SOAPEnvelope envelope(soap_ns);
      envelope.NewChild(authz_response);
      Arc::PayloadSOAP *outpayload = new Arc::PayloadSOAP(envelope);

      std::string tmp;
      outpayload->GetXML(tmp);
      std::cout<<"Output payload: "<<tmp<<std::endl;

      outmsg.Payload(outpayload);
    }

    if(resp) delete resp;
    
    return Arc::MCC_Status(Arc::STATUS_OK);
  } 
  else {
    delete inmsg.Payload();
    logger.msg(Arc::VERBOSE, "process: %s: not supported",method);
    return Arc::MCC_Status(); 
  }
  return Arc::MCC_Status();
}

Charon::Charon(Arc::Config *cfg):RegisteredService(cfg), logger_(Arc::Logger::rootLogger, "Charon"), eval(NULL) {
  logger_.addDestination(logcerr);
  // Define supported namespaces
  ns_["ra"]="http://www.nordugrid.org/schemas/request-arc";
  ns_["response"]="http://www.nordugrid.org/schemas/response-arc";
  ns_["pdp"]="http://www.nordugrid.org/schemas/pdp";

  //Load the Evaluator object
  evaluator_name_ = (std::string)((*cfg)["Evaluator"].Attribute("name"));
  logger.msg(Arc::INFO, "Evaluator: %s", evaluator_name_);

  //Get the policy location, and put it into evaluator
  Arc::XMLNode policystore = (*cfg)["PolicyStore"];
  Arc::XMLNode location = policystore["Location"];
  for(;(bool)location;++location) {
    std::string policyfile = location;
    std::string autoreload = location.Attribute("AutoReload");
    bool autoreload_b = true;
    if((autoreload == "false") || (autoreload == "0")) autoreload_b = false;
    if(policyfile.empty()) continue;
    locations_.push_back(PolicyLocation(policyfile,autoreload_b));
    logger.msg(Arc::INFO, "Policy location: %s", policyfile);
  }
  load_policies();
}

bool Charon::policies_modified(void) {
  for(std::list<PolicyLocation>::iterator p = locations_.begin();
                   p != locations_.end(); ++p) {
    if(p->IsModified()) return true;
  }
  return false;
}

bool Charon::load_policies(void) {
  ArcSec::EvaluatorLoader eval_loader;
  if(eval) delete eval;
  eval = eval_loader.getEvaluator(evaluator_name_);
  if(eval == NULL) {
    logger.msg(Arc::ERROR, "Can not dynamically produce Evaluator");
    return false;
  }
  else logger.msg(Arc::INFO, "Succeeded to produce Evaluator");
  for(std::list<PolicyLocation>::iterator p = locations_.begin();
                   p != locations_.end(); ++p) {
    logger.msg(Arc::VERBOSE, "Loading policy from %s",p->path);
    SourceFile source(p->path);
    if(!source) {
      logger.msg(Arc::ERROR, "Failed loading policy from %s",p->path);
      delete eval;
      eval = NULL;
      return false;
    }
    eval->addPolicy(SourceFile(p->path));
  }
  return true;
}


Charon::~Charon(void) {
  if(eval)
    delete eval;
  eval = NULL;
}

bool Charon::RegistrationCollector(Arc::XMLNode &doc) {
  Arc::NS isis_ns; isis_ns["isis"] = "http://www.nordugrid.org/schemas/isis/2008/08";
  Arc::XMLNode regentry(isis_ns, "RegEntry");
  regentry.NewChild("SrcAdv").NewChild("Type") = "org.nordugrid.security.charon";
  regentry.New(doc);
  return true;
}

Charon::PolicyLocation::PolicyLocation(const std::string& location,bool autoreload) {
  reload = autoreload;
  struct stat st;
  if(stat(location.c_str(), &st) != 0) return;
  if(!S_ISREG(st.st_mode)) return;
  mtime = st.st_mtime;
  ctime = st.st_ctime;
  path = location;
}

bool Charon::PolicyLocation::IsModified(void) {
  if(!reload) return false;
  logger.msg(Arc::DEBUG, "Checking policy modification: %s",path);
  if(path.empty()) return false;
  struct stat st;
  if((stat(path.c_str(), &st) != 0) || (!S_ISREG(st.st_mode))) {
    logger.msg(Arc::INFO, "Policy removed: %s", path);
    return true;
  }
  logger.msg(Arc::DEBUG, "Old policy times: %u/%u",(unsigned int)mtime,(unsigned int)ctime);
  logger.msg(Arc::DEBUG, "New policy times: %u/%u",(unsigned int)st.st_mtime,(unsigned int)st.st_ctime);
  if((mtime == st.st_mtime) && (ctime == st.st_ctime)) return false;
  mtime = st.st_mtime;
  ctime = st.st_ctime;
  logger.msg(Arc::INFO, "Policy modified: %s", path);
  return true;
}


} // namespace ArcSec

Arc::PluginDescriptor PLUGINS_TABLE_NAME[] = {
    { "charon", "HED:SERVICE", 0, &ArcSec::get_service },
    { NULL, NULL, 0, NULL }
};

