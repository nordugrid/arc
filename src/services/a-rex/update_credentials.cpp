#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/message/SOAPEnvelope.h>
#include <arc/ws-addressing/WSA.h>
#include "job.h"

#include "arex.h"

namespace ARex {


Arc::MCC_Status ARexService::UpdateCredentials(ARexGMConfig& config,Arc::XMLNode in,Arc::XMLNode out,const std::string& credentials) {
  /*
  UpdateCredentials (deleg)
    DelegatedToken
      Reference (multiple)

  UpdateCredentialsResponse (deleg)

  NotAuthorizedFault
  InvalidRequestMessageFault
  InvalidActivityIdentifierFault
  */
  {
    std::string s;
    in.GetXML(s);
    logger_.msg(Arc::VERBOSE, "UpdateCredentials: request = \n%s", s);
  };
  // Extract job id from references
  Arc::XMLNode refnode = in["DelegatedToken"]["Reference"];
  if(!refnode) {
    // Must refer to job
    logger_.msg(Arc::ERROR, "UpdateCredentials: missing Reference");
    Arc::SOAPFault fault(out.Parent(),Arc::SOAPFault::Sender,"Must have Activity specified in Reference");
    InvalidRequestMessageFault(fault,"arcdeleg:Reference","Wrong multiplicity");
    out.Destroy();
    return Arc::MCC_Status();
  }
  if((bool)(refnode[1])) {
    // Only one job can be updated per operation (profile)
    logger_.msg(Arc::ERROR, "UpdateCredentials: wrong number of Reference");
    Arc::SOAPFault fault(out.Parent(),Arc::SOAPFault::Sender,"Can update credentials only for single Activity");
    InvalidRequestMessageFault(fault,"arcdeleg:Reference","Wrong multiplicity");
    out.Destroy();
    return Arc::MCC_Status();
  };
  if(refnode.Size() != 1) {
    // Expecting single job EPR in Reference
    logger_.msg(Arc::ERROR, "UpdateCredentials: wrong number of elements inside Reference");
    Arc::SOAPFault fault(out.Parent(),Arc::SOAPFault::Sender,"Can update credentials only for single Activity");
    InvalidRequestMessageFault(fault,"arcdeleg:Reference","Wrong content");
    out.Destroy();
    return Arc::MCC_Status();
  }
  std::string jobid = Arc::WSAEndpointReference(refnode.Child()).ReferenceParameters()["a-rex:JobID"];
  if(jobid.empty()) {
    // EPR is wrongly formatted or not an A-REX EPR
    logger_.msg(Arc::ERROR, "UpdateCredentials: EPR contains no JobID");
    Arc::SOAPFault fault(out.Parent(),Arc::SOAPFault::Sender,"Can't find JobID element in ActivityIdentifier");
    InvalidRequestMessageFault(fault,"arcdeleg:Reference","Wrong content");
    out.Destroy();
    return Arc::MCC_Status();
  };
  ARexJob job(jobid,config,logger_);
  if(!job) {
    // There is no such job
    std::string failure = job.Failure();
    logger_.msg(Arc::ERROR, "UpdateCredentials: no job found: %s",failure);
    Arc::SOAPFault fault(out.Parent(),Arc::SOAPFault::Sender,"Can't find requested Activity");
    UnknownActivityIdentifierFault(fault,"No corresponding Activity found");
    out.Destroy();
    return Arc::MCC_Status();
  };
  if(!job.UpdateCredentials(credentials)) {
    logger_.msg(Arc::ERROR, "UpdateCredentials: failed to update credentials");
    Arc::SOAPFault fault(out.Parent(),Arc::SOAPFault::Sender,"Internal error: Failed to update credentials");
    out.Destroy();
    return Arc::MCC_Status();
  };
  {
    std::string s;
    out.GetXML(s);
    logger_.msg(Arc::VERBOSE, "UpdateCredentials: response = \n%s", s);
  };
  return Arc::MCC_Status(Arc::STATUS_OK);
}

} // namespace ARex

