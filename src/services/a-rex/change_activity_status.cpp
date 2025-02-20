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
        logger_.msg(Arc::ERROR, "A-REX REST: Failed to resume job");
        return Arc::MCC_Status(Arc::STATUS_OK);
      };
    } else {
      logger_.msg(Arc::ERROR, "A-REX REST: State change not allowed: from %s to %s", gm_state, new_state);
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

