#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/message/SOAPEnvelope.h>
#include <arc/ws-addressing/WSA.h>
#include "job.h"

#include "arex.h"

namespace ARex {


Arc::MCC_Status ARexService::GetActivityDocuments(ARexGMConfig& config,Arc::XMLNode in,Arc::XMLNode out) {
  /*
  GetActivityDocuments
    ActivityIdentifier (wsa:EndpointReferenceType, unbounded)

  GetActivityDocumentsResponse
    Response (unbounded)
      ActivityIdentifier
      JobDefinition (jsdl:JobDefinition)
      Fault (soap:Fault)

  */
  {
    std::string s;
    in.GetXML(s);
    logger.msg(Arc::DEBUG, "GetActivityDocuments: request = \n%s", s.c_str());
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
    // Read JSDL of job
    Arc::XMLNode jsdl = resp.NewChild("bes-factory:JobDefinition");
    if(!job.GetDescription(jsdl)) {
      // Processing failure
      jsdl.Destroy();

      continue;
    };
    jsdl.Name("bes-factory:JobDefinition"); // Recovering namespace of element
  };
  {
    std::string s;
    out.GetXML(s);
    logger.msg(Arc::DEBUG, "GetActivityDocuments: response = \n%s", s.c_str());
  };
  return Arc::MCC_Status(Arc::STATUS_OK);
}

} // namespace ARex

