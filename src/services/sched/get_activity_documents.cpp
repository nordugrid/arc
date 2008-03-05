#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "grid_sched.h"

namespace GridScheduler {


Arc::MCC_Status GridSchedulerService::GetActivityDocuments(Arc::XMLNode &in,Arc::XMLNode &out) {
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
    std::string jobid = Arc::WSAEndpointReference(id).ReferenceParameters()["sched:JobID"];
    if(jobid.empty()) {
      continue;
    };

    if(!sched_queue.CheckJobID(jobid)) {
       logger_.msg(Arc::ERROR, "GetActivityStatuses: job %s", jobid.c_str());
       Arc::SOAPEnvelope fault(ns_,true);
       fault.Fault()->Code(Arc::SOAPFault::Sender);
       fault.Fault()->Reason("Unknown activity");
       Arc::XMLNode f = fault.Fault()->Detail(true).NewChild("bes-factory:UnknownActivityIdentifierFault");
       out.Replace(fault.Child());
       return Arc::MCC_Status();
    }

    /*
    // TODO: Check permissions on that ID
    */

    // Read JSDL of job
    Arc::XMLNode jsdl = resp.NewChild("bes-factory:JobDefinition");

    jsdl = sched_queue.getJob(jobid).getJSDL();
    jsdl.Name("bes-factory:JobDefinition"); // Recovering namespace of element
  };
  {
    std::string s;
    out.GetXML(s);
    logger_.msg(Arc::DEBUG, "GetActivityDocuments: response = \n%s", s.c_str());
  };
  return Arc::MCC_Status(Arc::STATUS_OK);
}

} // namespace
