#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/message/SOAPEnvelope.h>
#include <arc/ws-addressing/WSA.h>
#include "job.h"

#include "arex.h"

namespace ARex {


Arc::MCC_Status ARexService::TerminateActivities(ARexGMConfig& config,Arc::XMLNode in,Arc::XMLNode out) {
  /*
  TerminateActivities
    ActivityIdentifier (wsa:EndpointReferenceType, unbounded)

  TerminateActivitiesResponse
    Response (unbounded)
      ActivityIdentifier
      Cancelled (boolean)
      Fault (soap:Fault)

  */
  {
    std::string s;
    in.GetXML(s);
    logger.msg(Arc::DEBUG, "TerminateActivities: request = \n%s", s.c_str());
  };
  for(int n = 0;;++n) {
    Arc::XMLNode id = in["ActivityIdentifier"][n];
    if(!id) break;
    // Create place for response
    Arc::XMLNode resp = out.NewChild("bes-factory:Response");
    resp.NewChild(id);
    std::string jobid = Arc::WSAEndpointReference(id).ReferenceParameters()["a-rex:JobID"];
    if(jobid.empty()) {
      // EPR is wrongly formated or not an A-REX EPR

      continue;
    };
    // Look for obtained ID
    ARexJob job(jobid,config);
    if(!job) {
      // There is no such job

      continue;
    };
    /*
    // Check permissions on that ID
    */
    // Cancel job (put a mark)
    bool result = job.Cancel();
    if(result) {
      resp.NewChild("bes-factory:Cancelled")="true";
    } else {
      resp.NewChild("bes-factory:Cancelled")="false";
      // Or should it be a fault?
    };
  };
  {
    std::string s;
    out.GetXML(s);
    logger.msg(Arc::DEBUG, "TerminateActivities: response = \n%s", s.c_str());
  };
  return Arc::MCC_Status(Arc::STATUS_OK);
}

}; // namespace ARex

