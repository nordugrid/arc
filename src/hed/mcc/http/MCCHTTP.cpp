#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/XMLNode.h>
#include <arc/StringConv.h>
#include <arc/message/PayloadRaw.h>
#include <arc/message/SecAttr.h>
#include <arc/loader/Plugin.h>
#include <arc/Utils.h>

#include "PayloadHTTP.h"
#include "MCCHTTP.h"



Arc::Logger ArcMCCHTTP::MCC_HTTP::logger(Arc::Logger::getRootLogger(), "MCC.HTTP");

ArcMCCHTTP::MCC_HTTP::MCC_HTTP(Arc::Config *cfg,Arc::PluginArgument* parg) : Arc::MCC(cfg,parg) {
}

static Arc::Plugin* get_mcc_service(Arc::PluginArgument* arg) {
    Arc::MCCPluginArgument* mccarg =
            arg?dynamic_cast<Arc::MCCPluginArgument*>(arg):NULL;
    if(!mccarg) return NULL;
    return new ArcMCCHTTP::MCC_HTTP_Service((Arc::Config*)(*mccarg),mccarg);
}

static Arc::Plugin* get_mcc_client(Arc::PluginArgument* arg) {
    Arc::MCCPluginArgument* mccarg =
            arg?dynamic_cast<Arc::MCCPluginArgument*>(arg):NULL;
    if(!mccarg) return NULL;
    return new ArcMCCHTTP::MCC_HTTP_Client((Arc::Config*)(*mccarg),mccarg);
}

extern Arc::PluginDescriptor const ARC_PLUGINS_TABLE_NAME[] = {
    { "http.service", "HED:MCC", NULL, 0, &get_mcc_service },
    { "http.client",  "HED:MCC", NULL, 0, &get_mcc_client  },
    { NULL, NULL, NULL, 0, NULL }
};

namespace ArcMCCHTTP {

using namespace Arc;

class HTTPSecAttr: public SecAttr {
 friend class MCC_HTTP_Service;
 friend class MCC_HTTP_Client;
 public:
  HTTPSecAttr(PayloadHTTPIn& payload);
  virtual ~HTTPSecAttr(void);
  virtual operator bool(void) const;
  virtual bool Export(SecAttrFormat format,XMLNode &val) const;
  virtual std::string get(const std::string& id) const;
  virtual std::map< std::string, std::list<std::string> > getAll() const;
 protected:
  std::string action_;
  std::string object_;
  virtual bool equal(const SecAttr &b) const;
};

HTTPSecAttr::HTTPSecAttr(PayloadHTTPIn& payload) {
  action_=payload.Method();
  std::string path = payload.Endpoint();
  // Remove service, port and protocol - those will be provided by
  // another layer
  std::string::size_type p = path.find("://");
  if(p != std::string::npos) {
    p=path.find('/',p+3);
    if(p != std::string::npos) {
      path.erase(0,p);
    };
  };
  object_=path;
}

HTTPSecAttr::~HTTPSecAttr(void) {
}

HTTPSecAttr::operator bool(void) const {
  return true;
}

std::string HTTPSecAttr::get(const std::string& id) const {
  if(id == "ACTION") return action_;
  if(id == "OBJECT") return object_;
  return "";
}

std::map<std::string, std::list<std::string> > HTTPSecAttr::getAll() const {
  static char const * const allIds[] = {
    "ACTION",
    "OBJECT",
    NULL
  };
  std::map<std::string, std::list<std::string> > all;
  for(char const * const * id = allIds; *id; ++id) {
    std::string idStr(*id);
    all[idStr] = SecAttr::getAll(idStr);
  }
  return all;
}

bool HTTPSecAttr::equal(const SecAttr &b) const {
  try {
    const HTTPSecAttr& a = (const HTTPSecAttr&)b;
    return ((action_ == a.action_) && (object_ == a.object_));
  } catch(std::exception&) { };
  return false;
}

bool HTTPSecAttr::Export(SecAttrFormat format,XMLNode &val) const {
  if(format == UNDEFINED) {
  } else if(format == ARCAuth) {
    NS ns;
    ns["ra"]="http://www.nordugrid.org/schemas/request-arc";
    val.Namespaces(ns); val.Name("ra:Request");
    XMLNode item = val.NewChild("ra:RequestItem");
    if(!object_.empty()) {
      XMLNode object = item.NewChild("ra:Resource");
      object=object_;
      object.NewAttribute("Type")="string";
      object.NewAttribute("AttributeId")="http://www.nordugrid.org/schemas/policy-arc/types/http/path";
    };
    if(!action_.empty()) {
      XMLNode action = item.NewChild("ra:Action");
      action=action_;
      action.NewAttribute("Type")="string";
      action.NewAttribute("AttributeId")="http://www.nordugrid.org/schemas/policy-arc/types/http/method";
    };
    return true;
  } else if(format == XACML) {
    NS ns;
    ns["ra"]="urn:oasis:names:tc:xacml:2.0:context:schema:os";
    val.Namespaces(ns); val.Name("ra:Request");
    if(!object_.empty()) {
      XMLNode object = val.NewChild("ra:Resource");
      XMLNode attr = object.NewChild("ra:Attribute");
      attr.NewChild("ra:AttributeValue") = object_;
      attr.NewAttribute("DataType")="xs:string";
      attr.NewAttribute("AttributeId")="http://www.nordugrid.org/schemas/policy-arc/types/http/path";
    };
    if(!action_.empty()) {
      XMLNode action = val.NewChild("ra:Action");
      XMLNode attr = action.NewChild("ra:Attribute");
      attr.NewChild("ra:AttributeValue") = action_;
      attr.NewAttribute("DataType")="xs:string";
      attr.NewAttribute("AttributeId")="http://www.nordugrid.org/schemas/policy-arc/types/http/method";
    };
    return true;
  } else {
  };
  return false;
}

MCC_HTTP_Service::MCC_HTTP_Service(Config *cfg,PluginArgument* parg):MCC_HTTP(cfg,parg) {
  XMLNode header_node = (*cfg)["Header"];
  while(header_node) {
    std::string header = (std::string)header_node;
    std::string::size_type pos = header.find(':');
    if (pos == std::string::npos) {
      headers_.push_back(std::make_pair(Arc::trim(header), std::string()));
    } else {
      headers_.push_back(std::make_pair(Arc::trim(header.substr(0,pos)), Arc::trim(header.substr(pos+1))));
    }
    ++header_node;
  }
}

MCC_HTTP_Service::~MCC_HTTP_Service(void) {
}

static MCC_Status make_http_fault(Logger& logger,
				  PayloadHTTPIn &inpayload,
				  PayloadStreamInterface& stream,
				  Message& outmsg,
				  int code,
				  std::list< std::pair<std::string,std::string> > const & headers,
				  const char* desc = NULL) {
  if((desc == NULL) || (*desc == 0)) {
    switch(code) {
      case HTTP_BAD_REQUEST:  desc="Bad Request"; break;
      case HTTP_NOT_FOUND:    desc="Not Found"; break;
      case HTTP_INTERNAL_ERR: desc="Internal error"; break;
      case HTTP_NOT_IMPLEMENTED: desc="Not Implemented"; break;
      default: desc="Something went wrong"; break;
    };
  };
  logger.msg(WARNING, "HTTP Error: %d %s",code,desc);
  PayloadHTTPOut outpayload(code,desc);
  bool keep_alive = (!inpayload)?false:inpayload.KeepAlive();
  outpayload.KeepAlive(keep_alive);
  // Add forced headers
  for(std::list< std::pair<std::string,std::string> >::const_iterator header = headers.cbegin();
      header != headers.cend();
      ++header) {
    outpayload.Attribute(header->first, header->second);
  }
  if(!outpayload.Flush(stream)) return MCC_Status();
  // Returning empty payload because response is already sent
  outmsg.Payload(new PayloadRaw);
  if(!keep_alive) return MCC_Status(SESSION_CLOSE);
  // If connection is supposed to be kept any unused body must be ignored
  if(!inpayload) return MCC_Status(SESSION_CLOSE);
  if(!inpayload.Sync()) return MCC_Status(SESSION_CLOSE);
  return MCC_Status(STATUS_OK);
}

static MCC_Status make_http_fault(Logger& logger,
				  PayloadHTTPIn &inpayload,
				  PayloadStreamInterface& stream,
				  Message& outmsg,
				  int code,
				  std::list< std::pair<std::string,std::string> > const & headers,
				  std::string const & desc) {
  return make_http_fault(logger, inpayload, stream, outmsg, code, headers, desc.empty()?"":desc.c_str());
}

static MCC_Status make_raw_fault(Message& outmsg,const char* desc = NULL) {
  PayloadRaw* outpayload = new PayloadRaw;
  if(desc) outpayload->Insert(desc,0);
  outmsg.Payload(outpayload);
  if(desc) return MCC_Status(GENERIC_ERROR,"HTTP",desc);
  return MCC_Status(GENERIC_ERROR,"HTTP");
}

static MCC_Status make_raw_fault(Message& outmsg,const std::string desc) {
  return make_raw_fault(outmsg,desc.c_str());
}

static MCC_Status make_raw_fault(Message& outmsg,const MCC_Status& desc) {
  PayloadRaw* outpayload = new PayloadRaw;
  std::string errstr = (std::string)desc;
  if(!errstr.empty()) outpayload->Insert(errstr.c_str(),0);
  outmsg.Payload(outpayload);
  return desc;
}

static void parse_http_range(PayloadHTTP& http,Message& msg) {
  std::string http_range = http.Attribute("range");
  if(http_range.empty()) return;
  if(strncasecmp(http_range.c_str(),"bytes=",6) != 0) return;
  std::string::size_type p = http_range.find(',',6);
  if(p != std::string::npos) {
    http_range=http_range.substr(6,p-6);
  } else {
    http_range=http_range.substr(6);
  };
  p=http_range.find('-');
  std::string val;
  if(p != std::string::npos) {
    val=http_range.substr(0,p);
    if(!val.empty()) msg.Attributes()->set("HTTP:RANGESTART",val);
    val=http_range.substr(p+1);
    if(!val.empty()) msg.Attributes()->set("HTTP:RANGEEND",val);
  };
}

MCC_Status MCC_HTTP_Service::process(Message& inmsg,Message& outmsg) {
  // Extracting payload
  if(!inmsg.Payload()) return MCC_Status();
  PayloadStreamInterface* inpayload = NULL;
  try {
    inpayload = dynamic_cast<PayloadStreamInterface*>(inmsg.Payload());
  } catch(std::exception& e) { };
  if(!inpayload) return MCC_Status();
  // Converting stream payload to HTTP which implements raw and stream interfaces
  PayloadHTTPIn nextpayload(*inpayload);
  if(!nextpayload) {
    logger.msg(WARNING, "Cannot create http payload");
    return make_http_fault(logger,nextpayload,*inpayload,outmsg,HTTP_BAD_REQUEST,headers_);
  };
  if(nextpayload.Method() == "END") {
    return MCC_Status(SESSION_CLOSE);
  };
  // By now PayloadHTTPIn only parsed header of incoming message.
  // If header contains Expect: 100-continue then intermediate
  // response must be returned to client followed by real response.
  if(nextpayload.AttributeMatch("expect", "100-continue")) {
    // For this request intermediate 100 response must be sent
    // TODO: maybe use PayloadHTTPOut for sending header.
    std::string oheader = "HTTP/1.1 100 CONTINUE\r\n\r\n"; // 100-continue happens only in version 1.1.
    if(!inpayload->Put(oheader)) {
      // Failed to send intermediate response.
      // Most probbaly connection was closed.
      return MCC_Status(SESSION_CLOSE);
    }
    // Now client will send body of message and it will become
    // available through PayloadHTTPIn
  }
  bool keep_alive = nextpayload.KeepAlive();
  // Creating message to pass to next MCC and setting new payload.
  Message nextinmsg = inmsg;
  nextinmsg.Payload(&nextpayload);
  // Creating attributes
  // Endpoints must be URL-like so make sure HTTP path is
  // converted to HTTP URL
  std::string endpoint = nextpayload.Endpoint();
  {
    std::string::size_type p = endpoint.find("://");
    if(p == std::string::npos) {
      // TODO: Use Host attribute of HTTP
      std::string oendpoint = nextinmsg.Attributes()->get("ENDPOINT");
      p=oendpoint.find("://");
      if(p != std::string::npos) {
        oendpoint.erase(0,p+3);
      };
      // Assuming we have host:port here
      if(oendpoint.empty() ||
         (oendpoint[oendpoint.length()-1] != '/')) {
        if(endpoint[0] != '/') oendpoint+="/";
      };
      // TODO: HTTPS detection
      endpoint="http://"+oendpoint+endpoint;
    };
  };
  nextinmsg.Attributes()->set("ENDPOINT",endpoint);
  nextinmsg.Attributes()->set("HTTP:ENDPOINT",nextpayload.Endpoint());
  nextinmsg.Attributes()->set("HTTP:METHOD",nextpayload.Method());
  bool request_is_head = (upper(nextpayload.Method()) == "HEAD");
  // Filling security attributes
  HTTPSecAttr* sattr = new HTTPSecAttr(nextpayload);
  nextinmsg.Auth()->set("HTTP",sattr);
  parse_http_range(nextpayload,nextinmsg);
  // Reason ?
  for(std::multimap<std::string,std::string>::const_iterator i =
      nextpayload.Attributes().begin();i!=nextpayload.Attributes().end();++i) {
    nextinmsg.Attributes()->add("HTTP:"+i->first,i->second);
  };
  {
    MCC_Status sret = ProcessSecHandlers(nextinmsg,"incoming");
    if(!sret) {
      return make_http_fault(logger,nextpayload,*inpayload,outmsg,HTTP_BAD_REQUEST,headers_,(std::string)sret); // Maybe not 400 ?
    };
  };
  // Call next MCC
  MCCInterface* next = Next(nextpayload.Method());
  if(!next) next = Next(); // try default target
  if(!next) {
    logger.msg(WARNING, "No next element in the chain");
    // Here selection is on method name. So failure result is "not supported"
    return make_http_fault(logger,nextpayload,*inpayload,outmsg,HTTP_NOT_IMPLEMENTED,headers_);
  }
  Message nextoutmsg = outmsg; nextoutmsg.Payload(NULL);
  MCC_Status ret = next->process(nextinmsg,nextoutmsg);
  // Do checks and extract raw response
  if(!ret) {
    if(nextoutmsg.Payload()) delete nextoutmsg.Payload();
    logger.msg(WARNING, "next element of the chain returned error status");
    int http_code = (ret.getKind() == UNKNOWN_SERVICE_ERROR)?HTTP_NOT_FOUND:HTTP_INTERNAL_ERR;
    // Check if next chain provided error code and reason
    std::string http_code_s = nextoutmsg.Attributes()->get("HTTP:CODE");
    std::string http_resp = nextoutmsg.Attributes()->get("HTTP:REASON");
    if (!http_code_s.empty()) stringto(http_code_s, http_code);
    return make_http_fault(logger,nextpayload,*inpayload,outmsg,http_code,headers_,http_resp.c_str());
  }
  if(!nextoutmsg.Payload()) {
    logger.msg(WARNING, "next element of the chain returned no payload");
    return make_http_fault(logger,nextpayload,*inpayload,outmsg,HTTP_INTERNAL_ERR,headers_);
  }
  PayloadRawInterface* retpayload = NULL;
  PayloadStreamInterface* strpayload = NULL;
  try {
    retpayload = dynamic_cast<PayloadRawInterface*>(nextoutmsg.Payload());
  } catch(std::exception& e) { };
  if(!retpayload) try {
    strpayload = dynamic_cast<PayloadStreamInterface*>(nextoutmsg.Payload());
  } catch(std::exception& e) { };
  if((!retpayload) && (!strpayload)) {
    logger.msg(WARNING, "next element of the chain returned invalid/unsupported payload");
    delete nextoutmsg.Payload();
    return make_http_fault(logger,nextpayload,*inpayload,outmsg,HTTP_INTERNAL_ERR,headers_);
  };
  {
    MCC_Status sret = ProcessSecHandlers(nextinmsg,"outgoing");
    if(!sret) {
      delete nextoutmsg.Payload();
      return make_http_fault(logger,nextpayload,*inpayload,outmsg,HTTP_BAD_REQUEST,headers_,(std::string)sret); // Maybe not 400 ?
    };
  };
  // Create HTTP response from raw body content
  // Use stream payload of inmsg to send HTTP response
  int http_code = HTTP_OK;
  std::string http_code_s = nextoutmsg.Attributes()->get("HTTP:CODE");
  std::string http_resp = nextoutmsg.Attributes()->get("HTTP:REASON");
  if(http_resp.empty()) http_resp = "OK";
  if(!http_code_s.empty()) stringto(http_code_s,http_code);
  nextoutmsg.Attributes()->removeAll("HTTP:CODE");
  nextoutmsg.Attributes()->removeAll("HTTP:REASON");
/*
  int l = 0;
  if(retpayload) {
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
  } else {
    if((strpayload->Pos() != 0) || (strpayload->Limit() != strpayload->Size())) {
      http_code=HTTP_PARTIAL;
      http_resp="Partial content";
    };
  };
*/
  PayloadHTTPOut* outpayload = NULL;
  PayloadHTTPOutRaw* routpayload = NULL;
  PayloadHTTPOutStream* soutpayload = NULL;
  if(retpayload) {
    routpayload = new PayloadHTTPOutRaw(http_code,http_resp,request_is_head);
    outpayload = routpayload;
  } else {
    soutpayload = new PayloadHTTPOutStream(http_code,http_resp,request_is_head);
    outpayload = soutpayload;
  };
  // Add forced headers
  for(std::list< std::pair<std::string,std::string> >::iterator header = headers_.begin(); header != headers_.end(); ++header) {
    outpayload->Attribute(header->first, header->second);
  }
  // Use attributes which higher level MCC may have produced for HTTP
  for(AttributeIterator i = nextoutmsg.Attributes()->getAll();i.hasMore();++i) {
    const char* key = i.key().c_str();
    if(strncmp("HTTP:",key,5) == 0) {
      key+=5;
      // TODO: check for special attributes: method, code, reason, endpoint, etc.
      outpayload->Attribute(std::string(key),*i);
    };
  };
  outpayload->KeepAlive(keep_alive);
  if(retpayload) {
    routpayload->Body(*retpayload);
  } else {
    soutpayload->Body(*strpayload);
  }
  bool flush_r = outpayload->Flush(*inpayload);
  delete outpayload;
  outmsg = nextoutmsg;
  // Returning empty payload because response is already sent through Flush
  // TODO: add support for non-stream communication through chain.
  outmsg.Payload(new PayloadRaw);
  if(!flush_r) {
    // If flush failed then we can't know if anything HTTPish was 
    // already sent. Hence we are just making lower level close
    // connection.
    logger.msg(WARNING, "Error to flush output payload");
    return MCC_Status(SESSION_CLOSE);
  };
  if(!keep_alive) return MCC_Status(SESSION_CLOSE);
  // Make sure whole body sent to us was fetch from input stream.
  if(!nextpayload.Sync()) return MCC_Status(SESSION_CLOSE);
  return MCC_Status(STATUS_OK);
}

MCC_HTTP_Client::MCC_HTTP_Client(Config *cfg,PluginArgument* parg):MCC_HTTP(cfg,parg) {
  endpoint_=(std::string)((*cfg)["Endpoint"]);
  method_=(std::string)((*cfg)["Method"]);
  authorization_=(std::string)((*cfg)["Authorization"]);
}

MCC_HTTP_Client::~MCC_HTTP_Client(void) {
}

static MCC_Status extract_http_response(Message& nextoutmsg, Message& outmsg, bool is_head, PayloadHTTPIn * & outpayload) {
  // Do checks and process response - supported response so far is stream
  // Generated result is HTTP payload with Raw and Stream interfaces
  // Check if any payload in message
  if(!nextoutmsg.Payload()) {
    return make_raw_fault(outmsg,"No response received by HTTP layer");
  };
  // Check if payload is stream (currently no other payload kinds are supported)
  PayloadStreamInterface* retpayload = NULL;
  try {
    retpayload = dynamic_cast<PayloadStreamInterface*>(nextoutmsg.Payload());
  } catch(std::exception& e) { };
  if(!retpayload) {
    delete nextoutmsg.Payload();
    return make_raw_fault(outmsg,"HTTP layer got something that is not stream");
  };
  // Try to parse payload. At least header part.
  outpayload  = new PayloadHTTPIn(*retpayload,true,is_head);
  if(!outpayload) {
    delete retpayload;
    return make_raw_fault(outmsg,"Returned payload is not recognized as HTTP");
  };
  if(!(*outpayload)) {
    std::string errstr = "Returned payload is not recognized as HTTP: "+outpayload->GetError();
    delete outpayload; outpayload = NULL;
    return make_raw_fault(outmsg,errstr.c_str());
  };
  // Check for closed connection during response - not suitable in client mode
  if(outpayload->Method() == "END") {
    delete outpayload; outpayload = NULL;
    return make_raw_fault(outmsg,"Connection was closed");
  };
  return MCC_Status(STATUS_OK);
}


MCC_Status MCC_HTTP_Client::process(Message& inmsg,Message& outmsg) {
  // Take payload, add HTTP stuf by using PayloadHTTPOut and
  // pass further through chain.
  // Extracting payload
  if(!inmsg.Payload()) return make_raw_fault(outmsg,"Nothing to send");
  PayloadRawInterface* inrpayload = NULL;
  PayloadStreamInterface* inspayload = NULL;
  try {
    inrpayload = dynamic_cast<PayloadRawInterface*>(inmsg.Payload());
  } catch(std::exception& e) { };
  try {
    inspayload = dynamic_cast<PayloadStreamInterface*>(inmsg.Payload());
  } catch(std::exception& e) { };
  if((!inrpayload) && (!inspayload)) return make_raw_fault(outmsg,"Notihing to send");
  // Making HTTP request
  // Use attributes which higher level MCC may have produced for HTTP
  std::string http_method = inmsg.Attributes()->get("HTTP:METHOD");
  std::string http_endpoint = inmsg.Attributes()->get("HTTP:ENDPOINT");
  if(http_method.empty()) http_method=method_;
  if(http_endpoint.empty()) http_endpoint=endpoint_;
  AutoPointer<PayloadHTTPOutRaw> nextrpayload(inrpayload?new PayloadHTTPOutRaw(http_method,http_endpoint):NULL);
  AutoPointer<PayloadHTTPOutStream> nextspayload(inspayload?new PayloadHTTPOutStream(http_method,http_endpoint):NULL);
  PayloadHTTPOut* nextpayload(inrpayload?
      dynamic_cast<PayloadHTTPOut*>(nextrpayload.Ptr()):
      dynamic_cast<PayloadHTTPOut*>(nextspayload.Ptr())
  );
  bool expect100 = false;
  bool authorization_present = false;
  for(AttributeIterator i = inmsg.Attributes()->getAll();i.hasMore();++i) {
    const char* key = i.key().c_str();
    if(strncmp("HTTP:",key,5) == 0) {
      key+=5;
      // TODO: check for special attributes: method, code, reason, endpoint, etc.
      if(strcasecmp(key,"METHOD") == 0) continue;
      if(strcasecmp(key,"ENDPOINT") == 0) continue;
      if(strcasecmp(key,"EXPECT") == 0) {
        if(Arc::lower(*i) == "100-continue") expect100 = true;
      }
      if(strcasecmp(key,"AUTHORIZATION") == 0) authorization_present = true;
      nextpayload->Attribute(std::string(key),*i);
    };
  };
  if(!authorization_present) {
    if(!authorization_.empty()) nextpayload->Attribute("Authorization", authorization_);
  };
  nextpayload->Attribute("User-Agent","ARC");
  bool request_is_head = (upper(http_method) == "HEAD");
  // Creating message to pass to next MCC and setting new payload..
  Message nextinmsg = inmsg;
  if(inrpayload) {
    nextrpayload->Body(*inrpayload,false);
    nextinmsg.Payload(nextrpayload.Ptr());
  } else {
    nextspayload->Body(*inspayload,false);
    nextinmsg.Payload(nextspayload.Ptr());
  };

  // Call next MCC
  MCCInterface* next = Next();
  if(!next) return make_raw_fault(outmsg,"Chain has no continuation");
  Message nextoutmsg = outmsg; nextoutmsg.Payload(NULL);
  MCC_Status ret;
  PayloadHTTPIn* outpayload  = NULL;
  if(!expect100) {
    // Simple request and response
    ret = next->process(nextinmsg,nextoutmsg);
    if(!ret) {
      delete nextoutmsg.Payload();
      return make_raw_fault(outmsg,ret);
    };
    ret = extract_http_response(nextoutmsg, outmsg, request_is_head, outpayload);
    if(!ret) return ret;
    // TODO: handle 100 response sent by server just in case
  } else {
    // Header and body must be sent separately with intermediate server
    // response fetched after header is sent.
    // Turning body off and sending header only
    nextpayload->ResetOutput(true,false);
    ret = next->process(nextinmsg,nextoutmsg);
    if(!ret) {
      delete nextoutmsg.Payload();
      return make_raw_fault(outmsg,ret);
    };
    // Parse response and check if it is 100
    ret = extract_http_response(nextoutmsg, outmsg, request_is_head, outpayload);
    if(!ret) return ret;
    int resp_code = outpayload->Code();
    if(resp_code == HTTP_CONTINUE) {
      // So continue with body
      delete outpayload; outpayload = NULL;
      nextpayload->ResetOutput(false,true);
      ret = next->process(nextinmsg,nextoutmsg);
      if(!ret) {
        delete nextoutmsg.Payload();
        return make_raw_fault(outmsg,ret);
      };
      ret = extract_http_response(nextoutmsg, outmsg, request_is_head, outpayload);
      if(!ret) return ret;
    } else {
      // Any other response should mean server can't accept request.
      // But just in case server responded with 2xx this can fool our caller.
      if(HTTP_CODE_IS_GOOD(resp_code)) {
        // Convert positive response into something bad
        std::string reason = outpayload->Reason();
        delete outpayload; outpayload = NULL;
        return make_raw_fault(outmsg,"Unexpected positive response received: "+
                                     Arc::tostring(resp_code)+" "+reason);
      }
    }
  }
  // Here outpayload should contain real response
  outmsg = nextoutmsg;
  // Payload returned by next.process is not destroyed here because
  // it is now owned by outpayload.
  outmsg.Payload(outpayload);
  outmsg.Attributes()->set("HTTP:CODE",tostring(outpayload->Code()));
  outmsg.Attributes()->set("HTTP:REASON",outpayload->Reason());
  outmsg.Attributes()->set("HTTP:KEEPALIVE",outpayload->KeepAlive()?"TRUE":"FALSE");
  for(std::map<std::string,std::string>::const_iterator i =
      outpayload->Attributes().begin();i!=outpayload->Attributes().end();++i) {
    outmsg.Attributes()->add("HTTP:"+i->first,i->second);
  };
  return MCC_Status(STATUS_OK);
}

} // namespace ArcMCCHTTP

