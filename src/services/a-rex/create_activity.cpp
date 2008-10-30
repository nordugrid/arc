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

  // HPC Basic Profile 1.0 comply (these fault handlings are defined in the KnowARC standards 
  // conformance roadmap 2nd release)

  Arc::XMLNode check;
  Arc::NS jsdl_namespaces;

  jsdl_namespaces[""] = "http://schemas.ggf.org/jsdl/2005/11/jsdl";
  jsdl_namespaces["jsdl-posix"] = "http://schemas.ggf.org/jsdl/2005/11/jsdl-posix";
  jsdl_namespaces["jsdl-arc"] = "http://www.nordugrid.org/ws/schemas/jsdl-arc";
  jsdl_namespaces["jsdl-hpcpa"] = "http://schemas.ggf.org/jsdl/2006/07/jsdl-hpcpa";

  check.Namespaces(jsdl_namespaces);

  check = in["ActivityDocument"]["JobDefinition"]["Application"]["POSIXApplication"]["WorkingDirectory"];

  if (check) {
    logger_.msg(Arc::ERROR, "jsdl-posix:WorkingDirectory: we do not support this element");
    Arc::SOAPFault fault(out.Parent(),Arc::SOAPFault::Sender,"The jsdl-posix:WorkingDirectory is part of the JSDL document");
    InvalidRequestMessageFault(fault,"jsdl-posix:WorkingDirectory","We do not support this element");
    out.Destroy();
    return Arc::MCC_Status();
  };

  check = in["ActivityDocument"]["JobDefinition"]["Application"]["HPCProfileApplication"]["WorkingDirectory"];

  if (check) {
    logger_.msg(Arc::ERROR, "jsdl-hpcpa:WorkingDirectory: we do not support this element");
    Arc::SOAPFault fault(out.Parent(),Arc::SOAPFault::Sender,"The jsdl-hpcpa:WorkingDirectory is part of the JSDL document");
    InvalidRequestMessageFault(fault,"jsdl-hpcpa:WorkingDirectory","We do not support this element");
    out.Destroy();
    return Arc::MCC_Status();
  };

  check = in["ActivityDocument"]["JobDefinition"]["Application"]["POSIXApplication"]["UserName"];

  if (check) {
    logger_.msg(Arc::ERROR, "jsdl-posix:UserName: we do not support this element");
    Arc::SOAPFault fault(out.Parent(),Arc::SOAPFault::Sender,"The jsdl-posix:UserName is part of the JSDL document");
    InvalidRequestMessageFault(fault,"jsdl-posix:UserName","We do not support this element");
    out.Destroy();
    return Arc::MCC_Status();
  };

  check = in["ActivityDocument"]["JobDefinition"]["Application"]["HPCProfileApplication"]["UserName"];

  if (check) {
    logger_.msg(Arc::ERROR, "jsdl-hpcpa:UserName: we do not support this element");
    Arc::SOAPFault fault(out.Parent(),Arc::SOAPFault::Sender,"The jsdl-hpcpa:UserName is part of the JSDL document");
    InvalidRequestMessageFault(fault,"jsdl-hpcpa:UserName","We do not support this element");
    out.Destroy();
    return Arc::MCC_Status();
  };

  check = in["ActivityDocument"]["JobDefinition"]["Application"]["POSIXApplication"]["GroupName"];

  if (check) {
    logger_.msg(Arc::ERROR, "jsdl-posix:GroupName: we do not support this element");
    Arc::SOAPFault fault(out.Parent(),Arc::SOAPFault::Sender,"The jsdl-posix:GroupName is part of the JSDL document");
    InvalidRequestMessageFault(fault,"jsdl-posix:GroupName","We do not support this element");
    out.Destroy();
    return Arc::MCC_Status();
  };

 // End of the HPC BP 1.0 fault handling part

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

