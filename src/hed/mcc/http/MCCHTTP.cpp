#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/XMLNode.h>
#include <arc/StringConv.h>
#include <arc/message/PayloadRaw.h>
#include <arc/loader/MCCLoader.h>

#include "PayloadHTTP.h"
#include "MCCHTTP.h"


Arc::Logger Arc::MCC_HTTP::logger(Arc::MCC::logger,"HTTP");

Arc::MCC_HTTP::MCC_HTTP(Arc::Config *cfg) : MCC(cfg) {
}

static Arc::MCC* get_mcc_service(Arc::Config *cfg,Arc::ChainContext*) {
    return new Arc::MCC_HTTP_Service(cfg);
}

static Arc::MCC* get_mcc_client(Arc::Config *cfg,Arc::ChainContext*) {
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

static MCC_Status make_http_fault(Arc::Logger& logger,PayloadStreamInterface& stream,Message& outmsg,int code,const char* desc = NULL) {
  if((desc == NULL) || (*desc == 0)) {
    switch(code) {
      case HTTP_BAD_REQUEST:  desc="Bad Request"; break;
      case HTTP_NOT_FOUND:    desc="Not Found"; break;
      case HTTP_INTERNAL_ERR: desc="Internal error"; break;
      default: desc="Something went wrong";
    };
  };
  logger.msg(WARNING, "HTTP Error: %d %s",code,desc);
  PayloadHTTP outpayload(code,desc,stream);
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
    logger.msg(WARNING, "Cannot create http payload"); 
    return make_http_fault(logger,*inpayload,outmsg,400);
  };
  if(nextpayload.Method() == "END") {
    return MCC_Status(SESSION_CLOSE);
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
  if(!ProcessSecHandlers(nextinmsg,"incoming")) {
    return make_http_fault(logger,*inpayload,outmsg, 400); // Maybe not 400 ?
  };
  // Call next MCC 
  MCCInterface* next = Next(nextpayload.Method());
  if(!next) {
    logger.msg(WARNING, "No next element in the chain");
    return make_http_fault(logger,*inpayload,outmsg, 404);
  }
  Message nextoutmsg = outmsg; nextoutmsg.Payload(NULL);
  MCC_Status ret = next->process(nextinmsg,nextoutmsg); 
  // Do checks and extract raw response
  if(!ret) {
    logger.msg(WARNING, "next element of the chain returned error status");
    return make_http_fault(logger,*inpayload,outmsg, 500);
  }
  if(!nextoutmsg.Payload()) {
    logger.msg(WARNING, "next element of the chain returned empty payload");
    return make_http_fault(logger,*inpayload,outmsg, 500);
  }
  PayloadRawInterface* retpayload = NULL;
  try {
    retpayload = dynamic_cast<PayloadRawInterface*>(nextoutmsg.Payload());
  } catch(std::exception& e) { };
  if(!retpayload) { 
    logger.msg(WARNING, "next element of the chain returned invalid payload");
    delete nextoutmsg.Payload(); 
    return make_http_fault(logger,*inpayload,outmsg,500); 
  };
  if(!ProcessSecHandlers(nextinmsg,"outgoing")) {
    delete nextoutmsg.Payload(); return make_http_fault(logger,*inpayload,outmsg,400); // Maybe not 400 ?
  };
  //if(!(*retpayload)) { delete retpayload; return make_http_fault(logger,*inpayload,outmsg); };
  // Create HTTP response from raw body content
  // Use stream payload of inmsg to send HTTP response
  //// TODO: make it possible for HTTP payload to acquire Raw payload to exclude double buffering
  int http_code = HTTP_OK;
  const char* http_resp = "OK";
  int l = 0;
  if(retpayload->BufferPos(0) != 0) {
    http_code=HTTP_PARTIAL;
    http_resp="Partial content";
  } else {
    for(int i = 0;;++i) {
      if(retpayload->Buffer(i) == NULL) break;
      l=retpayload->BufferPos(i) + retpayload->BufferSize(i);
    };
    if(l != retpayload->Size()) {
      http_code=HTTP_PARTIAL;
      http_resp="Partial content";
    };
  };
  PayloadHTTP* outpayload = new PayloadHTTP(http_code,http_resp,*inpayload);
  // Use attributes which higher level MCC may have produced for HTTP
  for(AttributeIterator i = nextoutmsg.Attributes()->getAll();i.hasMore();++i) {
    const char* key = i.key().c_str();
    if(strncmp("HTTP:",key,5) == 0) {
      key+=5;
      // TODO: check for special attributes: method, code, reason, endpoint, etc.
      outpayload->Attribute(std::string(key),*i);
    };
  };
  outpayload->Body(*retpayload);
  if(!outpayload->Flush()) {
    logger.msg(WARNING, "Error to flush output payload");
    return make_http_fault(logger,*inpayload,outmsg, 500);
  }
  delete outpayload;
  outmsg = nextoutmsg;
  // Returning empty payload because response is already sent through Flush
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
  // Use attributes which higher level MCC may have produced for HTTP
  std::string http_method = inmsg.Attributes()->get("HTTP:METHOD");
  std::string http_endpoint = inmsg.Attributes()->get("HTTP:ENDPOINT");
  if(http_method.empty()) http_method=method_;
  if(http_endpoint.empty()) http_endpoint=endpoint_;
  PayloadHTTP nextpayload(http_method,http_endpoint);
  for(AttributeIterator i = inmsg.Attributes()->getAll();i.hasMore();++i) {
    const char* key = i.key().c_str();
    if(strncmp("HTTP:",key,5) == 0) {
      key+=5;
      // TODO: check for special attributes: method, code, reason, endpoint, etc.
      if(strcmp(key,"METHOD") == 0) continue;
      if(strcmp(key,"ENDPOINT") == 0) continue;
      nextpayload.Attribute(std::string(key),*i);
    };
  };
  nextpayload.Attribute("User-Agent","ARC");
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
  Message nextoutmsg = outmsg; nextoutmsg.Payload(NULL);
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
  // Check for closed connection during response - not suitable in client mode
  if(outpayload->Method() == "END") { delete retpayload; delete outpayload; return make_raw_fault(outmsg); };
  outmsg = nextoutmsg;
  delete outmsg.Payload(outpayload);
  outmsg.Attributes()->set("HTTP:CODE",tostring(outpayload->Code()));
  outmsg.Attributes()->set("HTTP:REASON",outpayload->Reason());
  for(std::map<std::string,std::string>::const_iterator i =
      outpayload->Attributes().begin();i!=outpayload->Attributes().end();++i) {
    outmsg.Attributes()->set("HTTP:"+i->first,i->second);
  };
  return MCC_Status(Arc::STATUS_OK);
}

