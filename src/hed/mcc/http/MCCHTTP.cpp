#include "common/XMLNode.h"
#include "message/PayloadRaw.h"
#include "PayloadHTTP.h"
#include "loader/Loader.h"
#include "loader/MCCLoader.h"

#include "MCCHTTP.h"


Arc::Logger Arc::MCC_HTTP::logger(Arc::MCC::logger,"HTTP");

Arc::MCC_HTTP::MCC_HTTP(Arc::Config *cfg) : MCC(cfg) {
}

static Arc::MCC* get_mcc_service(Arc::Config *cfg,Arc::ChainContext* ctx) {
    return new Arc::MCC_HTTP_Service(cfg);
}

static Arc::MCC* get_mcc_client(Arc::Config *cfg,Arc::ChainContext* ctx) {
    return new Arc::MCC_HTTP_Client(cfg);
}

mcc_descriptors ARC_MCC_LOADER = {
    { "http.service", 0, &get_mcc_service },
    { "http.client",  0, &get_mcc_client },
    { NULL, 0, NULL }
};

using namespace Arc;


MCC_HTTP_Service::MCC_HTTP_Service(Arc::Config *cfg):MCC_HTTP(cfg) {
}

MCC_HTTP_Service::~MCC_HTTP_Service(void) {
}

static MCC_Status make_http_fault(PayloadStreamInterface& stream,Message& outmsg,const char* desc = NULL) {
  if((desc == NULL) || (*desc == 0)) desc="Bad Request";
  PayloadHTTP outpayload(400,desc,stream);
  outpayload.Flush();
  // Returning empty payload because response is already sent
  PayloadRaw* outpayload_e = new PayloadRaw;
  outmsg.Payload(outpayload_e);
  return MCC_Status();
}

static MCC_Status make_raw_fault(Message& outmsg,const char* desc = NULL) {
  PayloadRaw* outpayload = new PayloadRaw;
  if(desc) outpayload->Insert(desc,0);
  outmsg.Payload(outpayload);
  return MCC_Status();
}


MCC_Status MCC_HTTP_Service::process(Message& inmsg,Message& outmsg) {
  // Extracting payload
  if(!inmsg.Payload()) return MCC_Status();
  PayloadStreamInterface* inpayload = NULL;
  try {
    inpayload = dynamic_cast<PayloadStreamInterface*>(inmsg.Payload());
  } catch(std::exception& e) { };
  if(!inpayload) return MCC_Status();
  // Converting stream payload to HTTP which also implements raw interface
  PayloadHTTP nextpayload(*inpayload);
  if(!nextpayload) {
    return make_http_fault(*inpayload,outmsg);
  };
  // Creating message to pass to next MCC and setting new payload. 
  Message nextinmsg = inmsg;
  nextinmsg.Payload(&nextpayload);
  // Creating attributes
  nextinmsg.Attributes()->set("ENDPOINT",nextpayload.Endpoint());
  nextinmsg.Attributes()->set("HTTP:ENDPOINT",nextpayload.Endpoint());
  nextinmsg.Attributes()->set("HTTP:METHOD",nextpayload.Method());
  // Reason ?
  for(std::map<std::string,std::string>::const_iterator i = 
      nextpayload.Attributes().begin();i!=nextpayload.Attributes().end();++i) {
    nextinmsg.Attributes()->set("HTTP:"+i->first,i->second);
  };
  // Call next MCC 
  MCCInterface* next = Next(nextpayload.Method());
  if(!next) return make_http_fault(*inpayload,outmsg);
  Message nextoutmsg;
  MCC_Status ret = next->process(nextinmsg,nextoutmsg); 
  // Do checks and extract raw response
  if(!ret) return make_http_fault(*inpayload,outmsg);
  if(!nextoutmsg.Payload()) return make_http_fault(*inpayload,outmsg);
  PayloadRaw* retpayload = NULL;
  try {
    retpayload = dynamic_cast<PayloadRaw*>(nextoutmsg.Payload());
  } catch(std::exception& e) { };
  if(!retpayload) { delete nextoutmsg.Payload(); return make_http_fault(*inpayload,outmsg); };
  //if(!(*retpayload)) { delete retpayload; return make_http_fault(*inpayload,outmsg); };
  // Create HTTP response from raw body content
  // Use stream payload of inmsg to send HTTP response
  // TODO: make it possible for HTTP paylaod to acquire Raw payload to exclude double buffering
  PayloadHTTP* outpayload = new PayloadHTTP(200,"OK",*inpayload);
  int l = 0;
  for(int i = 0;;++i) {
    char* buf = retpayload->Buffer(i);
    if(!buf) break;
    int bufsize = retpayload->BufferSize(i);
    outpayload->Insert(buf,l,bufsize); l+=bufsize;
  };
  delete retpayload;
  if(!outpayload->Flush()) return make_http_fault(*inpayload,outmsg);
  delete outpayload;
  outmsg = nextoutmsg;
  // Returning empty payload because response is already sent
  PayloadRaw* outpayload_e = new PayloadRaw;
  outmsg.Payload(outpayload_e);
  return MCC_Status(Arc::STATUS_OK);
}

MCC_HTTP_Client::MCC_HTTP_Client(Arc::Config *cfg):MCC_HTTP(cfg) {
  endpoint_=(std::string)((*cfg)["Endpoint"]);
  method_=(std::string)((*cfg)["Method"]);
}

MCC_HTTP_Client::~MCC_HTTP_Client(void) {
}

MCC_Status MCC_HTTP_Client::process(Message& inmsg,Message& outmsg) {
  // Take Raw payload, add HTTP stuf by using PayloadHTTP and
  // generate new Raw payload to pass further through chain.
  // TODO: do not create new object - use or acqure same one.
  // Extracting payload
  if(!inmsg.Payload()) return make_raw_fault(outmsg);
  PayloadRawInterface* inpayload = NULL;
  try {
    inpayload = dynamic_cast<PayloadRawInterface*>(inmsg.Payload());
  } catch(std::exception& e) { };
  if(!inpayload) return make_raw_fault(outmsg);
  // Making HTTP request
  PayloadHTTP nextpayload(method_,endpoint_);
  // Bad solution - copying buffers between payloads
  int l = 0;
  for(int n = 0;;++n) {
    char* buf = inpayload->Buffer(n);
    if(!buf) break;
    int bufsize = inpayload->BufferSize(n);
    if(bufsize <= 0) continue;
    nextpayload.Insert(buf,l,bufsize);
    l+=bufsize;
  };
  nextpayload.Flush();
  // Creating message to pass to next MCC and setting new payload.. 
  Message nextinmsg = inmsg;
  nextinmsg.Payload(&nextpayload);
  // Call next MCC 
  MCCInterface* next = Next();
  if(!next) return make_raw_fault(outmsg);
  Message nextoutmsg;
  MCC_Status ret = next->process(nextinmsg,nextoutmsg); 
  // Do checks and process response - supported response so far is stream
  // Generated result is HTTP payload with Raw interface
  if(!ret) return make_raw_fault(outmsg);
  if(!nextoutmsg.Payload()) return make_raw_fault(outmsg);
  PayloadStreamInterface* retpayload = NULL;
  try {
    retpayload = dynamic_cast<PayloadStreamInterface*>(nextoutmsg.Payload());
  } catch(std::exception& e) { };
  if(!retpayload) { delete nextoutmsg.Payload(); return make_raw_fault(outmsg); };
  PayloadHTTP* outpayload  = new PayloadHTTP(*retpayload);
  if(!outpayload) { delete retpayload; return make_raw_fault(outmsg); };
  if(!(*outpayload)) { delete retpayload; delete outpayload; return make_raw_fault(outmsg); };
  outmsg = nextoutmsg;
  delete outmsg.Payload(outpayload);
  return MCC_Status(Arc::STATUS_OK);
}

