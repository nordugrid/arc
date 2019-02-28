#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/message/SOAPEnvelope.h>
#include <arc/ws-addressing/WSA.h>
#include <arc/message/PayloadStream.h>
#include <arc/message/PayloadRaw.h>
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

  NotAuthorizedFault
  InvalidActivityIdentifierFault
  CantApplyOperationToCurrentStateFault
  */
  {
    std::string s;
    in.GetXML(s);
    logger_.msg(Arc::VERBOSE, "ChangeActivityStatus: request = \n%s", s);
  };
  Arc::WSAEndpointReference id(in["ActivityIdentifier"]);
  if(!(Arc::XMLNode)id) {
    // Wrong request
    logger_.msg(Arc::ERROR, "ChangeActivityStatus: no ActivityIdentifier found");
    Arc::SOAPFault fault(out.Parent(),Arc::SOAPFault::Sender,"Can't find ActivityIdentifier element in request");
    InvalidRequestMessageFault(fault,"jsdl:ActivityIdentifier","Element is missing");
    out.Destroy();
    return Arc::MCC_Status(Arc::STATUS_OK);
  };
  std::string jobid = Arc::WSAEndpointReference(id).ReferenceParameters()["a-rex:JobID"];
  if(jobid.empty()) {
    // EPR is wrongly formated or not an A-REX EPR
    logger_.msg(Arc::ERROR, "ChangeActivityStatus: EPR contains no JobID");
    Arc::SOAPFault fault(out.Parent(),Arc::SOAPFault::Sender,"Can't find JobID element in ActivityIdentifier");
    InvalidRequestMessageFault(fault,"a-rex:JobID","Element is missing");
    out.Destroy();
    return Arc::MCC_Status(Arc::STATUS_OK);
  };
  ARexJob job(jobid,config,logger_);
  if(!job) {
    // There is no such job
    std::string failure = job.Failure();
    logger_.msg(Arc::ERROR, "ChangeActivityStatus: no job found: %s",failure);
    Arc::SOAPFault fault(out.Parent(),Arc::SOAPFault::Sender,"Can't find requested Activity");
    UnknownActivityIdentifierFault(fault,"No corresponding Activity found");
    out.Destroy();
    return Arc::MCC_Status(Arc::STATUS_OK);
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
    Arc::SOAPFault fault(out.Parent(),Arc::SOAPFault::Sender,"Missing NewStatus element in request");
    InvalidRequestMessageFault(fault,"a-rex:NewStatus","Element is missing");
    out.Destroy();
    return Arc::MCC_Status(Arc::STATUS_OK);
  };
  std::string new_bes_state = new_state.Attribute("state");
  std::string new_arex_state = new_state["a-rex:state"];
  // Take renewed proxy if supplied
  std::string delegation;
  Arc::XMLNode delegated_token = new_state["arcdeleg:DelegatedToken"];
  if(delegated_token) {
    if(!delegation_stores_.DelegatedToken(config.GmConfig().DelegationDir(),delegated_token,config.GridName(),delegation)) {
      // Failed to accept delegation (report as bad request)
      logger_.msg(Arc::ERROR, "ChangeActivityStatus: Failed to accept delegation");
      Arc::SOAPFault fault(out.Parent(),Arc::SOAPFault::Sender,"Failed to accept delegation");
      InvalidRequestMessageFault(fault,"arcdeleg:DelegatedToken","This token does not exist");
      out.Destroy();
      return Arc::MCC_Status(Arc::STATUS_OK);
    };
  };

  bool pending = false;
  std::string gm_state = job.State(pending);
  bool failed = job.Failed();
  std::string bes_state("");
  std::string arex_state("");
  convertActivityStatus(gm_state,bes_state,arex_state,failed,pending);
  // Old state in request must be checked against current one
  if((!old_bes_state.empty()) && (old_bes_state != bes_state)) {
    logger_.msg(Arc::ERROR, "ChangeActivityStatus: old BES state does not match");
    Arc::SOAPFault fault(out.Parent(),Arc::SOAPFault::Sender,"OldStatus is not same as current status");
    CantApplyOperationToCurrentStateFault(fault,gm_state,failed,"OldStatus does not match");
    out.Destroy();
    return Arc::MCC_Status(Arc::STATUS_OK);
  };
  if((!old_arex_state.empty()) && (old_arex_state != arex_state)) {
    logger_.msg(Arc::ERROR, "ChangeActivityStatus: old A-REX state does not match");
    Arc::SOAPFault fault(out.Parent(),Arc::SOAPFault::Sender,"OldStatus is not same as current status");
    CantApplyOperationToCurrentStateFault(fault,gm_state,failed,"OldStatus does not match");
    out.Destroy();
    return Arc::MCC_Status(Arc::STATUS_OK);
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
    if(!job.UpdateCredentials(delegation)) {
      logger_.msg(Arc::ERROR, "ChangeActivityStatus: Failed to update credentials");
      Arc::SOAPFault fault(out.Parent(),Arc::SOAPFault::Sender,"Internal error: Failed to update credentials");
      out.Destroy();
      return Arc::MCC_Status(Arc::STATUS_OK);
    };
    if(!job.Resume()) {
      logger_.msg(Arc::ERROR, "ChangeActivityStatus: Failed to resume job");
      Arc::SOAPFault fault(out.Parent(),Arc::SOAPFault::Sender,"Internal error: Failed to resume activity");
      out.Destroy();
      return Arc::MCC_Status(Arc::STATUS_OK);
    };
  } else {
    logger_.msg(Arc::ERROR, "ChangeActivityStatus: State change not allowed: from %s/%s to %s/%s",
                bes_state.c_str(),arex_state.c_str(),new_bes_state.c_str(),new_arex_state.c_str());
    Arc::SOAPFault fault(out.Parent(),Arc::SOAPFault::Sender,"Requested status transition is not supported");
    CantApplyOperationToCurrentStateFault(fault,gm_state,failed,"Requested status transition is not supported");
    out.Destroy();
    return Arc::MCC_Status(Arc::STATUS_OK);
  };
  // Make response
  // TODO: 
  // Updating currenst job state
  gm_state=job.State(pending);
  failed=job.Failed();
  convertActivityStatus(gm_state,bes_state,arex_state,failed,pending);
  Arc::XMLNode state = out.NewChild("a-rex:NewStatus");
  state.NewAttribute("bes-factory:state")=bes_state;
  state.NewChild("a-rex:state")=arex_state;
  {
    std::string s;
    out.GetXML(s);
    logger_.msg(Arc::VERBOSE, "ChangeActivityStatus: response = \n%s", s);
  };
  return Arc::MCC_Status(Arc::STATUS_OK);
}

#define MAX_ACTIVITIES (10000)

Arc::MCC_Status ARexService::ESPauseActivity(ARexGMConfig& config,Arc::XMLNode in,Arc::XMLNode out) {
  /*
    PauseActivity
      estypes:ActivityID 1-

    PauseActivityResponse
      PauseActivityResponseItem 1-
        estypes:ActivityID
        .
          EstimatedTime 0-1
          estypes:InternalBaseFault
          OperationNotPossibleFault
          OperationNotAllowedFault
          ActivityNotFoundFault
          AccessControlFault

    estypes:VectorLimitExceededFault
    estypes:AccessControlFault
    estypes:InternalBaseFault
   */
  Arc::XMLNode id = in["ActivityID"];
  unsigned int n = 0;
  for(;(bool)id;++id) {
    if((++n) > MAX_ACTIVITIES) {
      Arc::SOAPFault fault(out.Parent(),Arc::SOAPFault::Sender,"");
      ESVectorLimitExceededFault(fault,MAX_ACTIVITIES,"Too many ActivityID");
      out.Destroy();
      return Arc::MCC_Status(Arc::STATUS_OK);
    };
  };
  id = in["ActivityID"];
  for(;(bool)id;++id) {
    std::string jobid = id;
    Arc::XMLNode item = out.NewChild("esmanag:PauseActivityResponseItem");
    item.NewChild("estypes:ActivityID") = jobid;
    ARexJob job(jobid,config,logger_);
    if(!job) {
      // There is no such job
      logger_.msg(Arc::ERROR, "EMIES:PauseActivity: job %s - %s", jobid, job.Failure());
      ESActivityNotFoundFault(item.NewChild("dummy"),job.Failure());
    } else {
      // Pause not implemented
      logger_.msg(Arc::ERROR, "EMIES:PauseActivity: job %s - %s", jobid, "pause not implemented");
      ESOperationNotPossibleFault(item.NewChild("dummy"),"pause not implemented yet");
    };
  };
  return Arc::MCC_Status(Arc::STATUS_OK);
}

Arc::MCC_Status ARexService::ESResumeActivity(ARexGMConfig& config,Arc::XMLNode in,Arc::XMLNode out) {
  /*
    ResumeActivity
      estypes:ActivityID 1-

    ResumeActivityResponse
      ResumeActivityResponseItem 1-
        estypes:ActivityID
        .
          EstimatedTime 0-1
          estypes:InternalBaseFault
          OperationNotPossibleFault
          OperationNotAllowedFault
          ActivityNotFoundFault
          AccessControlFault

    estypes:VectorLimitExceededFault
    estypes:AccessControlFault
    estypes:InternalBaseFault
   */
  Arc::XMLNode id = in["ActivityID"];
  unsigned int n = 0;
  for(;(bool)id;++id) {
    if((++n) > MAX_ACTIVITIES) {
      Arc::SOAPFault fault(out.Parent(),Arc::SOAPFault::Sender,"");
      ESVectorLimitExceededFault(fault,MAX_ACTIVITIES,"Too many ActivityID");
      out.Destroy();
      return Arc::MCC_Status(Arc::STATUS_OK);
    };
  };
  id = in["ActivityID"];
  for(;(bool)id;++id) {
    std::string jobid = id;
    Arc::XMLNode item = out.NewChild("esmanag:ResumeActivityResponseItem");
    item.NewChild("estypes:ActivityID") = jobid;
    ARexJob job(jobid,config,logger_);
    if(!job) {
      // There is no such job
      logger_.msg(Arc::ERROR, "EMIES:ResumeActivity: job %s - %s", jobid, job.Failure());
      ESActivityNotFoundFault(item.NewChild("dummy"),job.Failure());
    } else {
      // Pause not implemented hence job can't be resumed too
      logger_.msg(Arc::ERROR, "EMIES:ResumeActivity: job %s - %s", jobid, "pause not implemented");
      ESOperationNotAllowedFault(item.NewChild("dummy"),"pause not implemented");
    };
  };
  return Arc::MCC_Status(Arc::STATUS_OK);
}

Arc::MCC_Status ARexService::ESCancelActivity(ARexGMConfig& config,Arc::XMLNode in,Arc::XMLNode out) {
  /*
    CancelActivity
      estypes:ActivityID 1-

    CancelActivityResponse
      CancelActivityResponseItem 1-
        estypes:ActivityID
        .
          EstimatedTime 0-1
          estypes:InternalBaseFault
          OperationNotPossibleFault
          OperationNotAllowedFault
          ActivityNotFoundFault
          AccessControlFault

    estypes:VectorLimitExceededFault
    estypes:AccessControlFault
    estypes:InternalBaseFault
   */
  Arc::XMLNode id = in["ActivityID"];
  unsigned int n = 0;
  for(;(bool)id;++id) {
    if((++n) > MAX_ACTIVITIES) {
      Arc::SOAPFault fault(out.Parent(),Arc::SOAPFault::Sender,"");
      ESVectorLimitExceededFault(fault,MAX_ACTIVITIES,"Too many ActivityID");
      out.Destroy();
      return Arc::MCC_Status(Arc::STATUS_OK);
    };
  };
  id = in["ActivityID"];
  for(;(bool)id;++id) {
    std::string jobid = id;
    Arc::XMLNode item = out.NewChild("esmanag:CancelActivityResponseItem");
    item.NewChild("estypes:ActivityID") = jobid;
    ARexJob job(jobid,config,logger_);
    if(!job) {
      // There is no such job
      logger_.msg(Arc::ERROR, "EMIES:CancelActivity: job %s - %s", jobid, job.Failure());
      ESActivityNotFoundFault(item.NewChild("dummy"),job.Failure());
    } else {
      if(!job.Cancel()) {
        // Probably wrong current state
        logger_.msg(Arc::ERROR, "EMIES:CancelActivity: job %s - %s", jobid, job.Failure());
        // TODO: check for real reason
        ESOperationNotAllowedFault(item.NewChild("dummy"),job.Failure());
      } else {
        // It may take "wakeup period" for cancel mark to be detected.
        // And same time till result of cancel script is processed.
        // Currently it is not possible to estimate how long canceling
        // would happen.
        logger_.msg(Arc::INFO, "job %s cancelled successfully", jobid);
        item.NewChild("esmanag:EstimatedTime") = Arc::tostring(config.GmConfig().WakeupPeriod()*2);
      };
    };
  };
  return Arc::MCC_Status(Arc::STATUS_OK);
}

Arc::MCC_Status ARexService::ESWipeActivity(ARexGMConfig& config,Arc::XMLNode in,Arc::XMLNode out) {
  /*
    esmanag:WipeActivity
      estypes:ActivityID

    esmanag:WipeActivityResponse
      esmanag:WipeActivityResponseItem
        estypes:ActivityID
        .
          esmanag:EstimatedTime (xsd:unsignedLong)
          estypes:InternalBaseFault
          OperationNotPossibleFault
          OperationNotAllowedFault
          ActivityNotFoundFault
          AccessControlFault

    estypes:VectorLimitExceededFault
    estypes:AccessControlFault
    estypes:InternalBaseFault
  */
  Arc::XMLNode id = in["ActivityID"];
  unsigned int n = 0;
  for(;(bool)id;++id) {
    if((++n) > MAX_ACTIVITIES) {
       Arc::SOAPFault fault(out.Parent(),Arc::SOAPFault::Sender,"");
      ESVectorLimitExceededFault(fault,MAX_ACTIVITIES,"Too many ActivityID");
      out.Destroy();
      return Arc::MCC_Status(Arc::STATUS_OK);
    };
  };
  id = in["ActivityID"];
  for(;(bool)id;++id) {
    std::string jobid = id;
    Arc::XMLNode item = out.NewChild("esmanag:WipeActivityResponseItem");
    item.NewChild("estypes:ActivityID") = jobid;
    ARexJob job(jobid,config,logger_);
    if(!job) {
      // There is no such job
      logger_.msg(Arc::ERROR, "EMIES:WipeActivity: job %s - %s", jobid, job.Failure());
      ESActivityNotFoundFault(item.NewChild("dummy"),job.Failure());
    } else {
      /*
        Despite it is against EMI-ES specs we allow job cleaning request to be accepted
        even if job is not in terminal state for user convenience.

      if((job.State() != "FINISHED") &&
         (job.State() != "DELETED")) {
        logger_.msg(Arc::ERROR, "EMIES:WipeActivity: job %s - state is %s, not terminal", jobid, job.State());
        ESOperationNotAllowedFault(item.NewChild("dummy"),"Not in terminal state");
      } else
      */
      if(!job.Clean()) {
        // Probably wrong current state
        logger_.msg(Arc::ERROR, "EMIES:WipeActivity: job %s - %s", jobid, job.Failure());
        // TODO: check for real reason
        ESOperationNotAllowedFault(item.NewChild("dummy"),job.Failure());
      } else {
        logger_.msg(Arc::INFO, "job %s (will be) cleaned successfully", jobid);
        item.NewChild("esmanag:EstimatedTime") = Arc::tostring(config.GmConfig().WakeupPeriod());
      };
    };
  };
  return Arc::MCC_Status(Arc::STATUS_OK);
}

Arc::MCC_Status ARexService::ESRestartActivity(ARexGMConfig& config,Arc::XMLNode in,Arc::XMLNode out) {
  /*
    esmanag:RestartActivity
      estypes:ActivityID

    esmanag:RestartActivityResponse
      esmanag:RestartActivityResponseItem
        estypes:ActivityID
        .
          esmanag:EstimatedTime (xsd:unsignedLong)
          estypes:InternalBaseFault
          OperationNotPossibleFault
          OperationNotAllowedFault
          ActivityNotFoundFault
          AccessControlFault

    estypes:VectorLimitExceededFault
    estypes:AccessControlFault
    estypes:InternalBaseFault
  */
  Arc::XMLNode id = in["ActivityID"];
  unsigned int n = 0;
  for(;(bool)id;++id) {
    if((++n) > MAX_ACTIVITIES) {
      Arc::SOAPFault fault(out.Parent(),Arc::SOAPFault::Sender,"");
      ESVectorLimitExceededFault(fault,MAX_ACTIVITIES,"Too many ActivityID");
      out.Destroy();
      return Arc::MCC_Status(Arc::STATUS_OK);
    };
  };
  id = in["ActivityID"];
  for(;(bool)id;++id) {
    std::string jobid = id;
    Arc::XMLNode item = out.NewChild("esmanag:RestartActivityResponseItem");
    item.NewChild("estypes:ActivityID") = jobid;
    ARexJob job(jobid,config,logger_);
    if(!job) {
      // There is no such job
      logger_.msg(Arc::ERROR, "EMIES:RestartActivity: job %s - %s", jobid, job.Failure());
      ESActivityNotFoundFault(item.NewChild("dummy"),job.Failure());
    } else {
      if(!job.Resume()) {
        // Probably wrong current state
        logger_.msg(Arc::ERROR, "EMIES:RestartActivity: job %s - %s", jobid, job.Failure());
        // TODO: check for real reason
        ESOperationNotAllowedFault(item.NewChild("dummy"),job.Failure());
      } else {
        logger_.msg(Arc::INFO, "job %s restarted successfully", jobid);
        item.NewChild("esmanag:EstimatedTime") = Arc::tostring(config.GmConfig().WakeupPeriod());
      };
    };
  };
  return Arc::MCC_Status(Arc::STATUS_OK);
}

Arc::MCC_Status ARexService::PutLogs(Arc::Message& inmsg,Arc::Message& outmsg,ARexGMConfig& config,std::string const& id,std::string const& subpath) {
  // Nothing can be put into root endpoint
  if(id.empty()) return make_http_fault(outmsg, 500, "No job specified");
  // Acquire job
  ARexJob job(id,config,logger_);
  if(!job) {
    // There is no such job
    logger_.msg(Arc::ERROR, "%s: there is no such job: %s", job.ID(), job.Failure());
    return make_http_fault(outmsg, 500, "Job does not exist");
  };
  if(subpath == "status") {
    // Request to change job state
    // Fetch content
    Arc::MessagePayload* payload = inmsg.Payload();
    if(!payload) {
      logger_.msg(Arc::ERROR, "%s: put log %s: there is no payload", id, subpath);
      return make_http_fault(outmsg,500,"Missing payload");
    };
    Arc::PayloadStreamInterface* stream = dynamic_cast<Arc::PayloadStreamInterface*>(payload);
    Arc::PayloadRawInterface* buf = dynamic_cast<Arc::PayloadRawInterface*>(payload);
    if((!stream) && (!buf)) {
      logger_.msg(Arc::ERROR, "%s: put log %s: unrecognized payload", id, subpath);
      return make_http_fault(outmsg, 500, "Error processing payload");
    }
    std::string new_state;
    static const int new_state_max_size = 256;
    if(stream) {
      std::string new_state_add_str;
      while(stream->Get(new_state_add_str)) {
        new_state.append(new_state_add_str);
        if(new_state.size() > new_state_max_size) break;
      }
    } else {
      for(unsigned int n = 0;buf->Buffer(n);++n) {
        new_state.append(buf->Buffer(n),buf->BufferSize(n));
        if(new_state.size() > new_state_max_size) break;
      };
    };
    new_state = Arc::upper(new_state);
    std::string gm_state = job.State();
    // Check for allowed combinations
    if(new_state == "FINISHED") {
      // Request to cancel job
      if((gm_state != "FINISHED") &&
         (gm_state != "CANCELING") &&
         (gm_state != "DELETED")) {
        job.Cancel();
      };
    } else if(new_state == "DELETED") {
      // Request to clean job
      if((gm_state != "FINISHED") &&
         (gm_state != "CANCELING") &&
         (gm_state != "DELETED")) {
        job.Cancel();
      };
      job.Clean();
    } else if((new_state == "PREPARING") || (new_state == "SUBMIT") ||
              (new_state == "INLRMS") || (new_state == "FINISHING")) {
      // Request to resume job
      if(!job.Resume()) {
        logger_.msg(Arc::ERROR, "ChangeActivityStatus: Failed to resume job");
        return Arc::MCC_Status(Arc::STATUS_OK);
      };
    } else {
      logger_.msg(Arc::ERROR, "ChangeActivityStatus: State change not allowed: from %s to %s", gm_state, new_state);
      return make_http_fault(outmsg, 500, "Impossible job state change request");
    };
    return make_http_fault(outmsg,200,"Done");
  }
  return make_http_fault(outmsg,500,"Requested operation is not possible");
}

Arc::MCC_Status ARexService::DeleteLogs(Arc::Message& inmsg,Arc::Message& outmsg,ARexGMConfig& config,std::string const& id,std::string const& subpath) {
  return make_http_fault(outmsg,501,"Not Implemented");
}

} // namespace ARex

