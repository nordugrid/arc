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
    logger_.msg(Arc::DEBUG, "ChangeActivityStatus: request = \n%s", s.c_str());
  };
  Arc::WSAEndpointReference id(in["ActivityIdentifier"]);
  if(!(Arc::XMLNode)id) {
    // Wrong request
    logger_.msg(Arc::ERROR, "ChangeActivityStatus: no ActivityIdentifier found");

    return Arc::MCC_Status();
  };
  std::string jobid = Arc::WSAEndpointReference(id).ReferenceParameters()["a-rex:JobID"];
  if(jobid.empty()) {
    // EPR is wrongly formated or not an A-REX EPR
    logger_.msg(Arc::ERROR, "ChangeActivityStatus: EPR contains no JobID");

    return Arc::MCC_Status();
  };
  ARexJob job(jobid,config);
  if(!job) {
    // There is no such job
    std::string failure = job.Failure();
    logger_.msg(Arc::ERROR, "ChangeActivityStatus: %s",failure.c_str());

    return Arc::MCC_Status();
  };

  // Old State
  Arc::XMLNode old_state = in["OldStatus"];
  std::string old_bes_state = old_state.Attribute("state");
  std::string old_arex_state = old_state["a-rex:state"];

  // New state
  Arc::XMLNode new_state = in["NewStatus"];
  if(!new_state) {
    // Wrong request
    logger_.msg(Arc::ERROR, "ChangeActivityStatus: missing NewStatus element");

    return Arc::MCC_Status();
  };
  std::string new_bes_state = new_state.Attribute("state");
  std::string new_arex_state = new_state["a-rex:state"];

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

  // Old state in request must be checked against current one
  if((!old_bes_state.empty()) && (old_bes_state != bes_state)) {
    logger_.msg(Arc::ERROR, "ChangeActivityStatus: old BES state does not match");

    return Arc::MCC_Status();
  };
  if((!old_arex_state.empty()) && (old_arex_state != arex_state)) {
    logger_.msg(Arc::ERROR, "ChangeActivityStatus: old A-Rex state does not match");

    return Arc::MCC_Status();
  };

  // Check for allowed combinations
  if((new_bes_state == "Finished") &&
     ((new_arex_state.empty()) || (new_arex_state == "Killing"))) {
    // Request to cancel job
    if((gm_state != "FINISHED") &&
       (gm_state != "CANCELING") &&
       (gm_state != "DELETED")) job.Cancel();
  } else
  if((new_bes_state == "Finished") &&
     (new_arex_state == "Deleted")) {
     // Request to clean job
    if((gm_state != "FINISHED") &&
       (gm_state != "CANCELING") &&
       (gm_state != "DELETED")) job.Cancel();
    job.Clean();
  } else 
  if((new_bes_state == "Running") &&
     (new_arex_state.empty())) { // Not supporting resume into user-defined state
    // Request to resume job
    if(!job.Resume()) {

      return Arc::MCC_Status();
    };
  } else {
    logger_.msg(Arc::ERROR, "ChangeActivityStatus: state change not allowed: from %s/%s to %s/%s",
                bes_state.c_str(),arex_state.c_str(),new_bes_state.c_str(),new_arex_state.c_str());

    return Arc::MCC_Status();
  };
  // Make response
  // TODO: 
  Arc::XMLNode state = out.NewChild("a-rex:NewStatus");
  state.NewAttribute("bes-factory:state")=bes_state;
  state.NewChild("a-rex:state")=arex_state;
  {
    std::string s;
    out.GetXML(s);
    logger_.msg(Arc::DEBUG, "ChangeActivityStatus: response = \n%s", s.c_str());
  };
  return Arc::MCC_Status(Arc::STATUS_OK);
}

} // namespace ARex

