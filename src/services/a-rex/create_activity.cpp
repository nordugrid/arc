#include <arc/message/SOAPEnvelope.h>
#include <arc/ws-addressing/WSA.h>
#include "job.h"

#include "arex.h"

namespace ARex {


Arc::MCC_Status ARexService::CreateActivity(ARexGMConfig& config,Arc::XMLNode in,Arc::XMLNode out,const std::string& clientid) {
  /*
  CreateActivity
    ActivityDocument
      jsdl:JobDefinition

  CreateActivityResponse
    ActivityIdentifier (wsa:EndpointReferenceType)
    ActivityDocument
      jsdl:JobDefinition

  NotAuthorizedFault
  NotAcceptingNewActivitiesFault
  UnsupportedFeatureFault
  InvalidRequestMessageFault
  */
  {
    std::string s;
    in.GetXML(s);
    logger_.msg(Arc::DEBUG, "CreateActivity: request = \n%s", s);
  };
  Arc::XMLNode jsdl = in["ActivityDocument"]["JobDefinition"];
  if(!jsdl) {
    // Wrongly formated request
    logger_.msg(Arc::ERROR, "CreateActivity: no job description found");
    Arc::SOAPFault fault(out.Parent(),Arc::SOAPFault::Sender,"Can't find JobDefinition element in request");
    InvalidRequestMessageFault(fault,"jsdl:JobDefinition","Element is missing");
    out.Destroy();
    return Arc::MCC_Status();
  };
  std::string delegation;
  Arc::XMLNode delegated_token = in["deleg:DelegatedToken"];
  if(delegated_token) {
    // Client wants to delegate credentials
    if(!delegations_.DelegatedToken(delegation,delegated_token)) {
      // Failed to accept delegation (report as bad request)
      logger_.msg(Arc::ERROR, "CreateActivity: Failed to accept delegation");
      Arc::SOAPFault fault(out.Parent(),Arc::SOAPFault::Sender,"Failed to accept delegation");
      InvalidRequestMessageFault(fault,"deleg:DelegatedToken","This token does not exist");
      out.Destroy();
      return Arc::MCC_Status();
    };
  };
  ARexJob job(jsdl,config,delegation,clientid,logger_);
  if(!job) {
    std::string failure = job.Failure();
    logger_.msg(Arc::ERROR, "CreateActivity: Failed to create new job: %s",failure);
    // Failed to create new job (no corresponding BES fault defined - using generic SOAP error)
    logger_.msg(Arc::ERROR, "CreateActivity: Failed to create new job");
    Arc::SOAPFault fault(out.Parent(),Arc::SOAPFault::Sender,("Failed to create new activity: "+failure).c_str());
    out.Destroy();
    return Arc::MCC_Status();
  };
  // Make SOAP response
  Arc::WSAEndpointReference identifier(out.NewChild("bes-factory:ActivityIdentifier"));
  // Make job's ID
  identifier.Address(config.Endpoint()); // address of service
  identifier.ReferenceParameters().NewChild("a-rex:JobID")=job.ID();
  identifier.ReferenceParameters().NewChild("a-rex:JobSessionDir")=config.Endpoint()+"/"+job.ID();
  out.NewChild(in["ActivityDocument"]);
  logger_.msg(Arc::DEBUG, "CreateActivity finished successfully");
  {
    std::string s;
    out.GetXML(s);
    logger_.msg(Arc::DEBUG, "CreateActivity: response = \n%s", s);
  };
  return Arc::MCC_Status(Arc::STATUS_OK);
}

} // namespace ARex

