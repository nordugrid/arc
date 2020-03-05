#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include <iostream>
#include <fstream>
#include <vector>

#include <arc/credential/VOMSUtil.h>

#include <openssl/err.h>
#include <openssl/rand.h>
#include <openssl/ssl.h>

#include <arc/message/PayloadStream.h>
#include <arc/message/PayloadRaw.h>
#include <arc/loader/Plugin.h>
#include <arc/message/MCCLoader.h>
#include <arc/XMLNode.h>
#include <arc/message/SecAttr.h>
#include <arc/crypto/OpenSSL.h>

#include "GlobusSigningPolicy.h"

#include "PayloadTLSStream.h"
#include "PayloadTLSMCC.h"

#include "DelegationCollector.h"

#include "MCCTLS.h"

namespace ArcMCCTLS {

using namespace Arc;

bool x509_to_string(X509* cert,std::string& str) {
  BIO *out = BIO_new(BIO_s_mem());
  if(!out) return false;
  if(!PEM_write_bio_X509(out,cert)) { BIO_free_all(out); return false; };
  for(;;) {
    char s[256];
    int l = BIO_read(out,s,sizeof(s));
    if(l <= 0) break;
    str.append(s,l);;
  };
  BIO_free_all(out);
  return true;
}

class TLSSecAttr: public SecAttr {
 friend class MCC_TLS_Service;
 friend class MCC_TLS_Client;
 public:
  TLSSecAttr(PayloadTLSStream&, ConfigTLSMCC& config, Logger& logger);
  virtual ~TLSSecAttr(void);
  virtual operator bool(void) const;
  virtual bool Export(SecAttrFormat format,XMLNode &val) const;
  std::string Identity(void) { return identity_; };
  std::string Subject(void) const {
    if(subjects_.size() <= 0) return "";
    return *(--(subjects_.end()));
  };
  std::string CA(void) const {
    if(subjects_.size() <= 0) return "";
    return *(subjects_.begin());
  };
  std::string X509Str(void) const {
    return x509str_;
  };
  std::string X509ChainStr(void) const {
    return x509chainstr_;
  };
  virtual std::string get(const std::string& id) const {
    if(id == "IDENTITY") return identity_;
    if(id == "SUBJECT") return Subject();
    if(id == "CA") return CA();
    if(id == "CERTIFICATE") return x509str_;
    if(id == "CERTIFICATECHAIN") return x509chainstr_;
    if(id == "LOCALSUBJECT") return target_;
    if((id == "VOMS") || (id == "VO")) {
      std::list<std::string> items = getAll(id);
      if(!items.empty()) return *items.begin();
      return "";
    };
    return "";
  };
  virtual std::list<std::string> getAll(const std::string& id) const {
    std::list<std::string> items;
    if(id == "VOMS") {
      for(std::vector<VOMSACInfo>::const_iterator v = voms_attributes_.begin();
                                   v != voms_attributes_.end();++v) { 
        for(std::vector<std::string>::const_iterator a = v->attributes.begin();
                                   a != v->attributes.end();++a) {
          std::string vattr = VOMSFQANToFull(v->voname,*a);
          items.push_back(vattr);
        };
      };
    } else if(id == "VO") {
      for(std::vector<VOMSACInfo>::const_iterator v = voms_attributes_.begin();
                                   v != voms_attributes_.end();++v) { 
        items.push_back(v->voname);
      };
    } else {
      return SecAttr::getAll(id);
    };
    return items;
  };
 protected:
  std::string identity_; // Subject of last non-proxy certificate
  std::list<std::string> subjects_; // Subjects of all certificates in chain
  std::vector<VOMSACInfo> voms_attributes_; // VOMS attributes from the VOMS extension of proxy
  std::string target_; // Subject of host certificate
  std::string x509str_; // The last certificate (in string format). TODO: extract on demand.
  std::string x509chainstr_; // Other certificates (in string format). TODO: extract on demand.
  bool processing_failed_;
  virtual bool equal(const SecAttr &b) const;
};

#define SELFSIGNED(cert) (X509_NAME_cmp(X509_get_issuer_name(cert),X509_get_subject_name(cert)) == 0)

TLSSecAttr::TLSSecAttr(PayloadTLSStream& payload, ConfigTLSMCC& config, Logger& logger) {
   std::string subject;
   processing_failed_ = false;
   STACK_OF(X509)* peerchain = payload.GetPeerChain();
   voms_attributes_.clear();
   if(peerchain != NULL) {
      for(int idx = 0;;++idx) {
         if(idx >= sk_X509_num(peerchain)) break;
         X509* cert = sk_X509_value(peerchain,sk_X509_num(peerchain)-idx-1);
         if(idx == 0) { // Obtain CA subject
           // Sometimes certificates chain contains CA certificate.
           if(!SELFSIGNED(cert)) {
             char* buf = X509_NAME_oneline(X509_get_issuer_name(cert),NULL,0);
             if(buf) {
               subject=buf;
               subjects_.push_back(subject);
               OPENSSL_free(buf);
             };
           };
         };
         {
           char* buf = X509_NAME_oneline(X509_get_subject_name(cert),NULL,0);
           if(buf) {
             subject=buf;
             subjects_.push_back(subject);
             OPENSSL_free(buf);
           };
         };
         std::string certstr;
         x509_to_string(cert, certstr);
         x509chainstr_=certstr+x509chainstr_;
         if(X509_get_ext_by_NID(cert,NID_proxyCertInfo,-1) < 0) {
            identity_=subject;
         };
         // Parse VOMS attributes from each certificate of the peer chain.
         Arc::VOMSTrustList trust_list(config.VOMSCertTrustDN());
         bool res = parseVOMSAC(cert, config.CADir(), config.CAFile(), config.VOMSDir(), trust_list, voms_attributes_, true, true);
         if(!res) {
            logger.msg(ERROR,"VOMS attribute parsing failed");
         };
      };
   };
   X509* peercert = payload.GetPeerCert();
   if (peercert != NULL) {
      if(subjects_.size() <= 0) { // Obtain CA subject if not obtained yet
        // Check for CA certificate used for connection - overprotection
        if(!SELFSIGNED(peercert)) {
          char* buf = X509_NAME_oneline(X509_get_issuer_name(peercert),NULL,0);
          if(buf) {
            subject=buf;
            subjects_.push_back(subject);
            OPENSSL_free(buf);
          };
        };
      };
      {
        char* buf = X509_NAME_oneline(X509_get_subject_name(peercert),NULL,0);
        if(buf) {
          subject=buf;
          //logger.msg(VERBOSE, "Peer name: %s", peer_dn);
          subjects_.push_back(subject);
          OPENSSL_free(buf);
        };
      };
      if(X509_get_ext_by_NID(peercert,NID_proxyCertInfo,-1) < 0) {
         identity_=subject;
      };
      // Parse VOMS attributes from peer certificate
      Arc::VOMSTrustList trust_list(config.VOMSCertTrustDN());
      bool res = parseVOMSAC(peercert, config.CADir(), config.CAFile(), config.VOMSDir(), trust_list, voms_attributes_, true, true);
      if(!res) {
        logger.msg(ERROR,"VOMS attribute parsing failed");
      };
      // Convert the x509 cert into string format
      x509_to_string(peercert, x509str_);
      X509_free(peercert);
   };
   if(identity_.empty()) identity_=subject;
   X509* hostcert = payload.GetCert();
   if (hostcert != NULL) {
      char* buf = X509_NAME_oneline(X509_get_subject_name(hostcert),NULL,0);
      if(buf) {
        target_=buf;
        //logger.msg(VERBOSE, "Host name: %s", peer_dn);
        OPENSSL_free(buf);
      };
   };
   // Cleaning collected VOMS attributes
   for(std::vector<VOMSACInfo>::iterator v = voms_attributes_.begin();
                                           v != voms_attributes_.end();) {
     if(v->status & VOMSACInfo::Error) {
       if(config.IfCheckVOMSCritical() && (v->status & VOMSACInfo::IsCritical)) {
         processing_failed_ = true; 
         logger.msg(ERROR,"Critical VOMS attribute processing failed");
       };
       if(config.IfFailOnVOMSParsing() && (v->status & VOMSACInfo::ParsingError)) {
         processing_failed_ = true; 
         logger.msg(ERROR,"VOMS attribute parsing failed");
       };
       if(config.IfFailOnVOMSInvalid() && (v->status & VOMSACInfo::ValidationError)) {
         processing_failed_ = true; 
         logger.msg(ERROR,"VOMS attribute validation failed");
       };
       logger.msg(WARNING,"VOMS attribute is ignored due to processing/validation error");
       v = voms_attributes_.erase(v);
     } else {
       ++v;
     };
   };
}

TLSSecAttr::~TLSSecAttr(void) {
}

TLSSecAttr::operator bool(void) const {
  return !processing_failed_;
}

bool TLSSecAttr::equal(const SecAttr &b) const {
  try {
    const TLSSecAttr& a = dynamic_cast<const TLSSecAttr&>(b);
    if (!a) return false;
    // ...
    return false;
  } catch(std::exception&) { };
  return false;
}

static void add_arc_subject_attribute(XMLNode item,const std::string& subject,const char* id) {
   XMLNode attr = item.NewChild("ra:SubjectAttribute");
   attr=subject; attr.NewAttribute("Type")="string";
   attr.NewAttribute("AttributeId")=id;
}

static void add_xacml_subject_attribute(XMLNode item,const std::string& subject,const char* id) {
   XMLNode attr = item.NewChild("ra:Attribute");
   attr.NewAttribute("DataType")="xs:string";
   attr.NewAttribute("AttributeId")=id;
   attr.NewChild("ra:AttributeValue") = subject;
}

bool TLSSecAttr::Export(SecAttrFormat format,XMLNode &val) const {
  if(format == UNDEFINED) {
  } else if(format == ARCAuth) {
    NS ns;
    ns["ra"]="http://www.nordugrid.org/schemas/request-arc";
    val.Namespaces(ns); val.Name("ra:Request");
    XMLNode item = val.NewChild("ra:RequestItem");
    XMLNode subj = item.NewChild("ra:Subject");
    std::list<std::string>::const_iterator s = subjects_.begin();
    std::string subject;
    if(s != subjects_.end()) {
      subject=*s;
      add_arc_subject_attribute(subj,subject,"http://www.nordugrid.org/schemas/policy-arc/types/tls/ca");
      for(;s != subjects_.end();++s) {
        subject=*s;
        add_arc_subject_attribute(subj,subject,"http://www.nordugrid.org/schemas/policy-arc/types/tls/chain");
      };
      add_arc_subject_attribute(subj,subject,"http://www.nordugrid.org/schemas/policy-arc/types/tls/subject");
    };
    if(!identity_.empty()) {
       add_arc_subject_attribute(subj,identity_,"http://www.nordugrid.org/schemas/policy-arc/types/tls/identity");
    };
    if(!voms_attributes_.empty()) {
      for(int k=0; k < voms_attributes_.size(); k++) {
        for(int n=0; n < voms_attributes_[k].attributes.size(); n++) {
          std::string vattr = VOMSFQANToFull(voms_attributes_[k].voname,voms_attributes_[k].attributes[n]);
          add_arc_subject_attribute(subj,vattr,"http://www.nordugrid.org/schemas/policy-arc/types/tls/vomsattribute");
        };
      };
    };
    if(!target_.empty()) {
      XMLNode resource = item.NewChild("ra:Resource");
      resource=target_; resource.NewAttribute("Type")="string";
      resource.NewAttribute("AttributeId")="http://www.nordugrid.org/schemas/policy-arc/types/tls/hostidentity";
      // Following is agreed to not be use till all use cases are clarified (Bern agreement)
      //hostidentity should be SubjectAttribute, because hostidentity is be constrained to access
      //the peer delegation identity, or some resource which is attached to the peer delegation identity.
      //The constrant is defined in delegation policy.
      //add_arc_subject_attribute(subj,target_,"http://www.nordugrid.org/schemas/policy-arc/types/tls/hostidentity");
    };
    return true;
  } else if(format == XACML) {
    NS ns;
    ns["ra"]="urn:oasis:names:tc:xacml:2.0:context:schema:os";
    val.Namespaces(ns); val.Name("ra:Request");
    XMLNode subj = val.NewChild("ra:Subject");
    std::list<std::string>::const_iterator s = subjects_.begin();
    std::string subject;
    if(s != subjects_.end()) {
      subject=*s;
      add_xacml_subject_attribute(subj,subject,"http://www.nordugrid.org/schemas/policy-arc/types/tls/ca");
      for(;s != subjects_.end();++s) {
        subject=*s;
        add_xacml_subject_attribute(subj,subject,"http://www.nordugrid.org/schemas/policy-arc/types/tls/chain");
      };
      add_xacml_subject_attribute(subj,subject,"http://www.nordugrid.org/schemas/policy-arc/types/tls/subject");
    };
    if(!identity_.empty()) {
       add_xacml_subject_attribute(subj,identity_,"http://www.nordugrid.org/schemas/policy-arc/types/tls/identity");
    };
    if(!voms_attributes_.empty()) {
      for(int k=0; k < voms_attributes_.size(); k++) {
        for(int n=0; n < voms_attributes_[k].attributes.size(); n++) {
          std::string vattr = VOMSFQANToFull(voms_attributes_[k].voname,voms_attributes_[k].attributes[n]);
          add_xacml_subject_attribute(subj,vattr,"http://www.nordugrid.org/schemas/policy-arc/types/tls/vomsattribute");
        };
      };
    };
    if(!target_.empty()) {
      XMLNode resource = val.NewChild("ra:Resource");
      XMLNode attr = resource.NewChild("ra:Attribute");
      attr.NewChild("ra:AttributeValue") = target_;
      attr.NewAttribute("DataType")="xs:string";
      attr.NewAttribute("AttributeId")="http://www.nordugrid.org/schemas/policy-arc/types/tls/hostidentity";
      // Following is agreed to not be use till all use cases are clarified (Bern agreement)
      //hostidentity should be SubjectAttribute, because hostidentity is be constrained to access
      //the peer delegation identity, or some resource which is attached to the peer delegation identity.
      //The constrant is defined in delegation policy.
      //add_xacml_subject_attribute(subj,target_,"http://www.nordugrid.org/schemas/policy-arc/types/tls/hostidentity");
    };
    return true;
  } else if(format == GACL) {
    NS ns;
    val.Namespaces(ns); val.Name("gacl");
    XMLNode entry = val.NewChild("entry");
    if(!identity_.empty()) entry.NewChild("person").NewChild("dn") = identity_;
    XMLNode voms;
    for(std::vector<VOMSACInfo>::const_iterator v = voms_attributes_.begin();
                                 v != voms_attributes_.end();++v) {
      for(std::vector<std::string>::const_iterator a = v->attributes.begin();
                                 a != v->attributes.end();++a) {
        if(!voms) voms = entry.NewChild("voms");
        voms.NewChild("fqan") = *a;
      };
      voms = XMLNode(); // ??
    };
    return true;
  } else {
  };
  return false;
}

class MCC_TLS_Context:public MessageContextElement {
 public:
  PayloadTLSMCC* stream;
  MCC_TLS_Context(PayloadTLSMCC* s = NULL):stream(s) { };
  virtual ~MCC_TLS_Context(void) { if(stream) delete stream; };
};

/* The main functionality of the constructor method is to
   initialize SSL layer. */
MCC_TLS_Service::MCC_TLS_Service(Config& cfg,PluginArgument* parg):MCC_TLS(cfg,false,parg) {
   if(!OpenSSLInit()) return;
}

MCC_TLS_Service::~MCC_TLS_Service(void) {
   // SSL deinit not needed
}

MCC_Status MCC_TLS_Service::process(Message& inmsg,Message& outmsg) {
   // Accepted payload is StreamInterface
   // Returned payload is undefined - currently holds no information

   // TODO: probably some other credentials check is needed
   //if(!sslctx_) return MCC_Status();

   // Obtaining underlying stream
   if(!inmsg.Payload()) return MCC_Status();
   PayloadStreamInterface* inpayload = NULL;
   try {
      inpayload = dynamic_cast<PayloadStreamInterface*>(inmsg.Payload());
   } catch(std::exception& e) { };
   if(!inpayload) return MCC_Status();

   // Obtaining previously created stream context or creating a new one
   MCC_TLS_Context* context = NULL;
   {
      MessageContextElement* mcontext = (*inmsg.Context())["tls.service"];
      if(mcontext) {
         try {
            context = dynamic_cast<MCC_TLS_Context*>(mcontext);
         } catch(std::exception& e) { };
      };
   };
   PayloadTLSMCC *stream = NULL;
   if(context) {
      // Old connection - using available SSL stream
      stream=context->stream;
   } else {
      // Creating new SSL object bound to stream of previous MCC
      // TODO: renew stream because it may be recreated by TCP MCC
      stream = new PayloadTLSMCC(inpayload,config_,logger);
      // Check for established connection
      if(!*stream) {
        logger.msg(ERROR, "Failed to establish connection: %s", stream->Failure().operator std::string());
        delete stream;
        return MCC_Status();
      }
      context=new MCC_TLS_Context(stream);
      inmsg.Context()->Add("tls.service",context);
   };

   // Creating message to pass to next MCC
   Message nextinmsg = inmsg;
   nextinmsg.Payload(stream);
   Message nextoutmsg = outmsg; nextoutmsg.Payload(NULL);

   PayloadTLSStream* tstream = dynamic_cast<PayloadTLSStream*>(stream);
   // Filling security attributes
   if(tstream) {
      TLSSecAttr* sattr = new TLSSecAttr(*tstream, config_, logger);
      if(sattr && *sattr) {
         //Getting the subject name of peer(client) certificate
         logger.msg(VERBOSE, "Peer name: %s", sattr->Subject());
         nextinmsg.Attributes()->set("TLS:PEERDN",sattr->Subject());
         logger.msg(VERBOSE, "Identity name: %s", sattr->Identity());
         nextinmsg.Attributes()->set("TLS:IDENTITYDN",sattr->Identity());
         logger.msg(VERBOSE, "CA name: %s", sattr->CA());
         nextinmsg.Attributes()->set("TLS:CADN",sattr->CA());
         if(!((sattr->target_).empty())) {
            nextinmsg.Attributes()->set("TLS:LOCALDN",sattr->target_);
         }
         nextinmsg.Auth()->set("TLS",sattr);
      } else {
         logger.msg(ERROR, "Failed to process security attributes in TLS MCC for incoming message");
         delete sattr;
         return MCC_Status();
      }
   }

   // Checking authentication and authorization;
   if(!ProcessSecHandlers(nextinmsg,"incoming")) {
      logger.msg(ERROR, "Security check failed in TLS MCC for incoming message");
      return MCC_Status();
   };

   // Call next MCC
   MCCInterface* next = Next();
   if(!next)  return MCC_Status();
   MCC_Status ret = next->process(nextinmsg,nextoutmsg);
   // TODO: If next MCC returns anything redirect it to stream
   if(nextoutmsg.Payload()) {
      delete nextoutmsg.Payload();
      nextoutmsg.Payload(NULL);
   };
   if(!ret) return ret;
   // For nextoutmsg, nothing to do for payload of msg, but
   // transfer some attributes of msg
   outmsg = nextoutmsg;
   return MCC_Status(STATUS_OK);
}

MCC_TLS_Client::MCC_TLS_Client(Config& cfg,PluginArgument* parg):MCC_TLS(cfg,true,parg){
   stream_=NULL;
   if(!OpenSSLInit()) return;
   /* Get DN from certificate, and put it into message's attribute */
}

MCC_TLS_Client::~MCC_TLS_Client(void) {
   if(stream_) delete stream_;
   // SSL deinit not needed
}

MCC_Status MCC_TLS_Client::process(Message& inmsg,Message& outmsg) {
   // Accepted payload is Raw and Stream
   // Returned payload is Stream
   // Extracting payload
   if(!inmsg.Payload()) {
     return MCC_Status(GENERIC_ERROR,"TLS","Internal error: missing payload for outgoing TLS message");
   }
   if(!stream_) {
     return MCC_Status(GENERIC_ERROR,"TLS","Internal error: communication stream not initialised");
   }
   if(!*stream_) {
     if(!stream_->Failure()) return stream_->Failure();
     return MCC_Status(GENERIC_ERROR,"TLS","Internal error: communication stream not initialised");
   }
   PayloadRawInterface* rinpayload = NULL;
   PayloadStreamInterface* sinpayload = NULL;
   try {
      rinpayload = dynamic_cast<PayloadRawInterface*>(inmsg.Payload());
   } catch(std::exception& e) { };
   try {
      sinpayload = dynamic_cast<PayloadStreamInterface*>(inmsg.Payload());
   } catch(std::exception& e) { };
   if((!rinpayload) && (!sinpayload)) {
     return MCC_Status(GENERIC_ERROR,"TLS","Internal error: unsupported payload for outgoing TLS message");
   }
   // Collecting security attributes
   // TODO: keep them or redo same for incoming message
   PayloadTLSStream* tstream = dynamic_cast<PayloadTLSStream*>(stream_);
   if(tstream) {
      TLSSecAttr* sattr = new TLSSecAttr(*tstream, config_, logger);
      inmsg.Auth()->set("TLS",sattr);
      //Getting the subject name of peer(client) certificate
      logger.msg(VERBOSE, "Peer name: %s", sattr->Subject());
      inmsg.Attributes()->set("TLS:PEERDN",sattr->Subject());
      logger.msg(VERBOSE, "Identity name: %s", sattr->Identity());
      inmsg.Attributes()->set("TLS:IDENTITYDN",sattr->Identity());
      logger.msg(VERBOSE, "CA name: %s", sattr->CA());
      inmsg.Attributes()->set("TLS:CADN",sattr->CA());
   }

   //Checking authentication and authorization;
   if(!ProcessSecHandlers(inmsg,"outgoing")) {
      logger.msg(VERBOSE, "Security check failed for outgoing TLS message");
      // TODO: propagate message from SecHandlers
      return MCC_Status(GENERIC_ERROR,"TLS","Security check failed for outgoing TLS message");
   };
   // Sending payload
   if(rinpayload) {
      for(int n=0;;++n) {
         char* buf = rinpayload->Buffer(n);
         if(!buf) break;
         int bufsize = rinpayload->BufferSize(n);
         if(!(stream_->Put(buf,bufsize))) {
            logger.msg(VERBOSE, "Failed to send content of buffer");
            if(!stream_->Failure()) return stream_->Failure();
            return MCC_Status(GENERIC_ERROR,"TLS","Failed to send content of buffer");
         };
      };
   } else {
        int size = -1;
        if(!sinpayload->Get(*stream_,size)) {
        // Currently false may also mean that stream finihsed. 
        // Hence it can't be used to indicate real failure.
        //    logger.msg(INFO, "Failed to transfer content of stream");
        //    return MCC_Status();
        };
   };
   PayloadTLSMCC* tlsstream = new PayloadTLSMCC(*stream_);
   outmsg.Payload(tlsstream);
   if(tlsstream && !*tlsstream)
      logger.msg(ERROR, "Failed to establish connection: %s", tlsstream->Failure().operator std::string());
   //outmsg.Attributes(inmsg.Attributes());
   //outmsg.Context(inmsg.Context());
   if(!ProcessSecHandlers(outmsg,"incoming")) {
      logger.msg(VERBOSE, "Security check failed for incoming TLS message");
      delete outmsg.Payload(NULL);
      // TODO: propagate message from SecHandlers
      return MCC_Status(GENERIC_ERROR,"TLS","Security check failed for incoming TLS message");
   };
   return MCC_Status(STATUS_OK);
}


void MCC_TLS_Client::Next(MCCInterface* next,const std::string& label) {
   if(label.empty()) {
      if(stream_) delete stream_;
      stream_=NULL;
      stream_=new PayloadTLSMCC(next,config_,logger);
      if(stream_ && !*stream_)
         logger.msg(ERROR, "Failed to establish connection: %s", stream_->Failure().operator std::string());
   };
   MCC::Next(next,label);
}


} // namespace ArcMCCTLS

//Glib::Mutex Arc::MCC_TLS::lock_;
Arc::Logger ArcMCCTLS::MCC_TLS::logger(Arc::Logger::getRootLogger(), "MCC.TLS");

ArcMCCTLS::MCC_TLS::MCC_TLS(Arc::Config& cfg,bool client,PluginArgument* parg) : Arc::MCC(&cfg,parg), config_(cfg,client) {
}

static Arc::Plugin* get_mcc_service(Arc::PluginArgument* arg) {
    Arc::MCCPluginArgument* mccarg =
            arg?dynamic_cast<Arc::MCCPluginArgument*>(arg):NULL;
    if(!mccarg) return NULL;
    return new ArcMCCTLS::MCC_TLS_Service(*(Arc::Config*)(*mccarg),mccarg);
}

static Arc::Plugin* get_mcc_client(Arc::PluginArgument* arg) {
    Arc::MCCPluginArgument* mccarg =
            arg?dynamic_cast<Arc::MCCPluginArgument*>(arg):NULL;
    if(!mccarg) return NULL;
    return new ArcMCCTLS::MCC_TLS_Client(*(Arc::Config*)(*mccarg),mccarg);
}

extern Arc::PluginDescriptor const ARC_PLUGINS_TABLE_NAME[] = {
    { "tls.service", "HED:MCC", NULL, 0, &get_mcc_service },
    { "tls.client",  "HED:MCC", NULL, 0, &get_mcc_client  },
    { "delegation.collector", "HED:SHC", NULL, 0, &ArcMCCTLSSec::DelegationCollector::get_sechandler},
    { NULL, NULL, NULL, 0, NULL }
};

