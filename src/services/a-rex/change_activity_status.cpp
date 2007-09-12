#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/message/SOAPEnvelope.h>
#include <arc/ws-addressing/WSA.h>
#include "job.h"

#include "arex.h"

namespace ARex {


Arc::MCC_Status ARexService::ChangeActivityStatus(ARexGMConfig& config,Arc::XMLNode in,Arc::XMLNode out) {
  /*
  ChangeActivityStatus
    ActivityIdentifier (wsa:EndpointReferenceType)
    OldStatus (a-rex,optional)
        attribute = state (bes-factory:ActivityStateEnumeration)
    NewStatus (a-rex)
        attribute = state (bes-factory:ActivityStateEnumeration)

  ChangeActivityStatusResponse
    NewStatus (a-rex)
        attribute = state (bes-factory:ActivityStateEnumeration)

  */
  {
    std::string s;
    in.GetXML(s);
    logger.msg(Arc::DEBUG, "ChangeActivityStatus: request = \n%s", s.c_str());
  };
  Arc::WSAEndpointReference id(in["ActivityIdentifier"]);
  if(!id) {
    // Wrong request

  };
  std::string jobid = Arc::WSAEndpointReference(id).ReferenceParameters()["a-rex:JobID"];
  if(jobid.empty()) {
    // EPR is wrongly formated or not an A-REX EPR

  };
  ARexJob job(jobid,config);
  if(!job) {
    // There is no such job

  };
  Arc::XMLNode old_state = in["OldStatus"];
  std::string old_bes_state = old_state.Attribute("ActivityStateEnumeration")
  std::string old_arex_state = old_state["a-rex:state"];
  // Old state must be checked against current one
  std::string gm_state = job.State();



  Arc::XMLNode new_state = in["NewStatus"];
  if(!new_state) {
    // Wrong request

  };
  std::string new_bes_state = old_state.Attribute("ActivityStateEnumeration")
  std::string new_arex_state = old_state["a-rex:state"];



  // Create place for response
  Arc::XMLNode resp = out.NewChild("a-rex:NewStatus");

    resp.NewChild(id);
    // Look for obtained ID
    ARexJob job(jobid,config);
    if(!job) {
      // There is no such job

      continue;
    };
    /*
    // Check permissions on that ID
    */
    std::string gm_state = job.State();
    std::string bes_state("");
    std::string arex_state("");
    if(gm_state == "ACCEPTED") {
      bes_state="Pending"; arex_state="Accepted";
    } else if(gm_state == "PREPARING") {
      bes_state="Running"; arex_state="Preparing";
    } else if(gm_state == "SUBMIT") {
      bes_state="Running"; arex_state="Submiting";
    } else if(gm_state == "INLRMS") {
      bes_state="Running"; arex_state="Executing";
    } else if(gm_state == "FINISHING") {
      bes_state="Running"; arex_state="Finishing";
    } else if(gm_state == "FINISHED") {
      bes_state="Finished"; arex_state="Finished";

    } else if(gm_state == "DELETED") {
      bes_state="Finished"; arex_state="Deleted";

    } else if(gm_state == "CANCELING") {
      bes_state="Running"; arex_state="Killing";
    };
    // Make response
    Arc::XMLNode state = resp.NewChild("bes-factor:ActivityStatus");
    state.NewAttribute("bes-factory:ActivityStateEnumeration")=bes_state;
    state.NewChild("a-rex:state")=arex_state;
  };
  {
    std::string s;
    out.GetXML(s);
    logger.msg(Arc::DEBUG, "ChangeActivityStatus: response = \n%s", s.c_str());
  };
  return Arc::MCC_Status(Arc::STATUS_OK);
}

}; // namespace ARex

