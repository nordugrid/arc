#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/message/SOAPEnvelope.h>
#include <arc/ws-addressing/WSA.h>
#include <arc/message/PayloadStream.h>
#include <arc/message/PayloadRaw.h>
#include <arc/XMLNode.h>
#include "job.h"

#include "arex.h"

namespace ARex {

Arc::MCC_Status ARexService::PutNew(Arc::Message& inmsg,Arc::Message& outmsg,ARexGMConfig& config,std::string const& subpath) {
  /*
    adl:ActivityDescription - http://www.eu-emi.eu/es/2010/12/adl
   */
  // subpath is ignored
  // Check for proper payload
  Arc::MessagePayload* payload = inmsg.Payload();
  if(!payload) {
    logger_.msg(Arc::ERROR, "NEW: put new job: there is no payload");
    return make_http_fault(outmsg,500,"Missing payload");
  };
  if(config.GmConfig().MaxTotal() > 0 && all_jobs_count_ >= config.GmConfig().MaxTotal()) {
    logger_.msg(Arc::ERROR, "NEW: put new job: max jobs total limit reached");
    return make_http_fault(outmsg,500,"No more jobs allowed");
  };
  // Fetch content 
  std::string desc_str;

  // TODO: Add job description size limit control

  Arc::MCC_Status res = ARexService::extract_content(inmsg,desc_str,100*1024*1024); // todo: add size control
  if(!res)
    return make_http_fault(outmsg,500,res.getExplanation().c_str());
  std::string clientid = (inmsg.Attributes()->get("TCP:REMOTEHOST"))+":"+(inmsg.Attributes()->get("TCP:REMOTEPORT"));
  // TODO: Do we need different generators for different formats?
  JobIDGeneratorES idgenerator(config.Endpoint());
  ARexJob job(desc_str,config,"","",clientid,logger_,idgenerator);
  if(!job) {
    return make_http_fault(outmsg,500,job.Failure().c_str());
  }; 
  return make_http_fault(outmsg,200,job.ID().c_str());
}

Arc::MCC_Status ARexService::DeleteNew(Arc::Message& inmsg,Arc::Message& outmsg,ARexGMConfig& config,std::string const& subpath) {
  return make_http_fault(outmsg,501,"Not Implemented");
}

} // namespace ARex

