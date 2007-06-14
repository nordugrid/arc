#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>

#include "../../libs/message/PayloadStream.h"
#include "../../libs/message/PayloadRaw.h"
#include "../../libs/loader/Loader.h"
#include "../../libs/loader/MCCLoader.h"
#include "../../../libs/common/XMLNode.h"

#include "PayloadTLSStream.h"
#include "PayloadTLSSocket.h"
#include "PayloadTLSMCC.h"

#include <openssl/err.h>
#include <openssl/rand.h>
#include <openssl/ssl.h>

#include "MCCTLS.h"

Arc::Logger Arc::MCC_TLS::logger(Arc::MCC::logger,"TLS");

Arc::MCC_TLS::MCC_TLS(Arc::Config *cfg) : MCC(cfg) {
}

static Arc::MCC* get_mcc_service(Arc::Config *cfg,Arc::ChainContext *ctx) {
    return new Arc::MCC_TLS_Service(cfg);
}

static Arc::MCC* get_mcc_client(Arc::Config *cfg,Arc::ChainContext *ctx) {
    return new Arc::MCC_TLS_Client(cfg);
}


mcc_descriptor __arc_mcc_modules__[] = {
    { "tls.service", 0, &get_mcc_service },
    { "tls.client", 0, &get_mcc_client },
    { NULL, 0, NULL }
};

using namespace Arc;


#ifndef TLS_ERROR_BUFSIZ
#define TLS_ERROR_BUFSIZ 2048
#endif
void tls_print_error(const char *fmt, ...){
   char errbuf[TLS_ERROR_BUFSIZ];
   va_list args;
   int r;
   va_start(args, fmt);
   bzero((char *)&errbuf, sizeof errbuf);
   vsnprintf(errbuf, sizeof errbuf, fmt, args);
   fputs(errbuf, stderr);
   //The error message can also be written to some log file
   va_end(args);
}

static void tls_process_error(){
   unsigned long err;
   err = ERR_get_error();
   if (err != 0)
   {
	tls_print_error("OpenSSL Error -- %s\n", ERR_error_string(err, NULL));
      	tls_print_error("Library  : %s\n", ERR_lib_error_string(err));
      	tls_print_error("Function : %s\n", ERR_func_error_string(err));
      	tls_print_error("Reason   : %s\n", ERR_reason_error_string(err));
   }
   return;
}

static int no_passphrase_callback(char *buf, int size, int rwflag, void *password){
   return -1;
}

static int tls_rand_seeded_p = 0;
#define my_MIN_SEED_BYTES 256 
bool MCC_TLS::tls_random_seed(std::string filename, size_t n)
{
   int r;
   r = RAND_load_file(filename.c_str(), (n > 0 && n < LONG_MAX) ? (long)n : LONG_MAX);
   if (n == 0)
	n = my_MIN_SEED_BYTES;
    if (r < n) {
        logger.msg(Arc::ERROR,
		   "tls_random_seed from file: could not read files");
   	tls_process_error();
	return false;
    } else {
	tls_rand_seeded_p = 1;
	return true;
    }
}

static DH *tls_dhe1024 = NULL; /* generating these takes a while, so do it just once */
static void tls_set_dhe1024()
{
   int i;
   RAND_bytes((unsigned char *) &i, sizeof i);
   if (i < 0)
       i = -i;
   DSA *dsaparams;
   DH *dhparams;
   const char *seed[] = { ";-)  :-(  :-)  :-(  ",
			   ";-)  :-(  :-)  :-(  ",
			   "Random String no. 12",
			   ";-)  :-(  :-)  :-(  ",
			   "hackers have even mo", /* from jargon file */
   };
   unsigned char seedbuf[20];
   if (i >= 0) {
	i %= sizeof seed / sizeof seed[0];
	memcpy(seedbuf, seed[i], 20);
	dsaparams = DSA_generate_parameters(1024, seedbuf, 20, NULL, NULL, 0, NULL);
    } else {
	/* random parameters (may take a while) */
	dsaparams = DSA_generate_parameters(1024, NULL, 0, NULL, NULL, 0, NULL);
    }
    if (dsaparams == NULL) {
	tls_process_error();
	return;
    }
    dhparams = DSA_dup_DH(dsaparams);
    DSA_free(dsaparams);
    if (dhparams == NULL) {
	tls_process_error();
	return;
    }
    if (tls_dhe1024 != NULL)
	DH_free(tls_dhe1024);
    	tls_dhe1024 = dhparams;
}

bool MCC_TLS::tls_load_certificate(SSL_CTX* sslctx, const std::string& cert_file, const std::string& key_file, const std::string& password, const std::string& random_file)
{
   // SSL_CTX_set_default_passwd_cb_userdata(sslctx_,password.c_str());
   SSL_CTX_set_default_passwd_cb(sslctx, no_passphrase_callback);  //Now, the authentication is based on no_passphrase credential, it would be modified later to add passphrase support.
   if((SSL_CTX_use_certificate_file(sslctx,cert_file.c_str(),
               SSL_FILETYPE_PEM) != 1) && 
      (SSL_CTX_use_certificate_file(sslctx,cert_file.c_str(),
               SSL_FILETYPE_ASN1) != 1)) {
        logger.msg(ERROR, "Can not load certificate file.");
        tls_process_error();
        return false;
   }
   if((SSL_CTX_use_PrivateKey_file(sslctx,key_file.c_str(),
               SSL_FILETYPE_PEM) != 1) &&
      (SSL_CTX_use_PrivateKey_file(sslctx,key_file.c_str(),
               SSL_FILETYPE_ASN1) != 1)) {
        logger.msg(ERROR, "Can not load key file.");
        tls_process_error();
        return false;
   }
   if(!(SSL_CTX_check_private_key(sslctx))) {
        logger.msg(ERROR, "Private key does not match certificate.");
        tls_process_error();
        return false;
   }
   if(tls_random_seed(random_file, 0)) {
     return false;
   }
}

bool MCC_TLS::do_ssl_init(void) {
   static bool ssl_inited = false;
   if(ssl_inited) return true;
   ssl_inited=true;
   SSL_load_error_strings();
   if(!SSL_library_init()){
        logger.msg(ERROR, "SSL_library_init failed.");
        tls_process_error();
        ssl_inited=false;
        return false;
   };
   return true;
}


/*The main functionality of the constructor method is creat SSL context object*/
MCC_TLS_Service::MCC_TLS_Service(Arc::Config *cfg):MCC_TLS(cfg) {
   std::string cert_file = (*cfg)["CertificatePath"];
   if(cert_file.empty()) cert_file="/etc/grid-security/hostcert.pem";
   std::string key_file = (*cfg)["KeyPath"];
   if(key_file.empty()) key_file="/etc/grid-security/hostkey.pem";
   std::string ca_file = (*cfg)["CACertificatePath"];
   std::string ca_dir = (*cfg)["CACertificatesDir"];
   if(ca_dir.empty()) ca_dir="/etc/grid-security/certificates";
   int r;
   if(!do_ssl_init()) return;
   /*Initialize the SSL Context object*/
   sslctx_=SSL_CTX_new(SSLv23_server_method());
   if(sslctx_==NULL){
        logger.msg(Arc::ERROR,
		   "Can not creat the SSL Context object.");
	tls_process_error();
	return;
   }
   SSL_CTX_set_mode(sslctx_,SSL_MODE_ENABLE_PARTIAL_WRITE);
   tls_load_certificate(sslctx_, cert_file, key_file, "", key_file);
   SSL_CTX_set_verify(sslctx_, SSL_VERIFY_PEER |  SSL_VERIFY_FAIL_IF_NO_PEER_CERT, NULL);
   if(!ca_file.empty()){
      r=SSL_CTX_load_verify_locations(sslctx_, ca_file.c_str(), NULL /* no CA-directory */);   //The CA-directory paremerters would be added to support multiple CA
      if(!r){
         tls_process_error();
         return;
      }   
      SSL_CTX_set_client_CA_list(sslctx_, 
         SSL_load_client_CA_file(ca_file.c_str())
      ); //Scan all certificates in CAfile and list them as acceptable CAs
      if(SSL_CTX_get_client_CA_list(sslctx_) == NULL){ 
         logger.msg(Arc::ERROR,
		   "Can not set client CA list from the specified file.");
   	 tls_process_error();
	 return;
      }
   }
   if(tls_dhe1024 == NULL){
   	tls_set_dhe1024();
	if(tls_dhe1024 == NULL){return;}
   }
   if (!SSL_CTX_set_tmp_dh(sslctx_, tls_dhe1024)){
           logger.msg(Arc::ERROR,
		   "DH set error.");
           tls_process_error();
	   return;
   }
   SSL_CTX_set_options(sslctx_, SSL_OP_SINGLE_DH_USE);
#ifndef NO_RSA
   RSA *tmpkey;
   tmpkey = RSA_generate_key(512, RSA_F4, 0, NULL);
   if (tmpkey == NULL)
	tls_process_error();
   if (!SSL_CTX_set_tmp_rsa(sslctx_, tmpkey)) {
	RSA_free(tmpkey);
	tls_process_error();
	return;
	}
   RSA_free(tmpkey);
#endif
   // The SSL object will be created when MCC_TCP_Service call 
   // the MCC_TLS_Service's process() method, and the SSL object 
   // can be reused just like socket object
}


MCC_TLS_Service::~MCC_TLS_Service(void) {
   if(sslctx_!=NULL)SSL_CTX_free(sslctx_);
}


class MCC_TLS_Context:public MessageContextElement {
 public:
  PayloadTLSSocket* stream;
  MCC_TLS_Context(PayloadTLSSocket* s = NULL):stream(s) { };
  ~MCC_TLS_Context(void) { if(stream) delete stream; };
};


MCC_Status MCC_TLS_Service::process(Message& inmsg,Message& outmsg) {
   // MCC_TCP_Service ---> MCC_TLS_Service ---> MCC_HTTP_Service ---> MCC_SOAP_Service
   // Accepted payload is Stream - not a StreamInterface, needed for 
   // otaining handle from it.
   // Returned payload is undefined - currently holds no information
   if(!inmsg.Payload()) return MCC_Status();
   PayloadStream* inpayload = NULL;
   try {
      	inpayload = dynamic_cast<PayloadStream*>(inmsg.Payload());
   } catch(std::exception& e) { };
   if(!inpayload) return MCC_Status();
   // Obtaining previously created stream or creating a new one
   PayloadTLSSocket *nextpayload = NULL;
   MCC_TLS_Context* context = NULL;
   {   
      MessageContextElement* mcontext = (*inmsg.Context())["tls.service"];
      if(mcontext) {
         try {
            context = dynamic_cast<MCC_TLS_Context*>(mcontext);
         } catch(std::exception& e) { };
      };
   };
   if(context) {
      nextpayload=context->stream;
   } else {
      context=new MCC_TLS_Context;
      inmsg.Context()->Add("tls.service",context);
   };
   if(!nextpayload) {
      // Adding ssl to socket stream, the "ssl" object is created and 
      // binded to socket fd in PayloadTLSSocket
      // TODO: create ssl object only once per connection. - done ?
      nextpayload = new PayloadTLSSocket(*inpayload, sslctx_, false, logger);
      context->stream=nextpayload;
   };
   if(!nextpayload) return MCC_Status();
 
   // Creating message to pass to next MCC
   Message nextinmsg = inmsg;
   nextinmsg.Payload(nextpayload);
   Message nextoutmsg;

   //Getting the subject name of peer(client) certificate
   X509* peercert;
   char buf[100];     
   peercert = (dynamic_cast<PayloadTLSStream*>(nextpayload))->GetPeercert();
   X509_NAME_oneline(X509_get_subject_name(peercert),buf,sizeof buf);
   std::string peer_dn = buf;
   std::cerr<< "DN name:\n"<<peer_dn<< std::endl;
   //Putting the subject name into nextoutmsg.Attribute; so far, the subject is put into Attribut temporally, it should be put into MessageAuth later.
   nextinmsg.Attributes()->set("TLS:PEERDN",peer_dn);
  

   // Call next MCC 
   MCCInterface* next = Next();
   if(!next) { 
      //delete nextpayload;
      return MCC_Status();
   };
   MCC_Status ret = next->process(nextinmsg,nextoutmsg);
   if(nextoutmsg.Payload()) {
      delete nextoutmsg.Payload();
      nextoutmsg.Payload(NULL);
   };
   if(!ret) {
      //delete nextpayload;
      return MCC_Status();
   };
   // For nextoutmsg, nothing to do for payload of msg, but 
   // transfer some attributes of msg
   outmsg = nextoutmsg;
   //delete nextpayload;
   return MCC_Status(Arc::STATUS_OK);
}

MCC_TLS_Client::MCC_TLS_Client(Arc::Config *cfg):MCC_TLS(cfg){
   stream_=NULL;
   std::string cert_file = (*cfg)["CertificatePath"];
   if(cert_file.empty()) cert_file="cert.pem";
   std::string key_file = (*cfg)["KeyPath"];
   if(key_file.empty()) key_file="key.pem";
   std::string ca_file = (*cfg)["CACertificatePath"];
   if(ca_file.empty()) ca_file="ca.pem";
   std::string ca_dir = (*cfg)["CACertificatesDir"];
   if(ca_dir.empty()) ca_dir="/etc/grid-security/certificates";
   int r;
   if(!do_ssl_init()) return;
   /*Initialize the SSL Context object*/
   sslctx_=SSL_CTX_new(SSLv23_client_method());
   if(sslctx_==NULL){
        logger.msg(ERROR, "Can not creat the SSL Context object");
        tls_process_error();
        return;
   }
   SSL_CTX_set_mode(sslctx_,SSL_MODE_ENABLE_PARTIAL_WRITE);
   tls_load_certificate(sslctx_, cert_file, key_file, "", key_file);
   SSL_CTX_set_verify(sslctx_, SSL_VERIFY_PEER |  SSL_VERIFY_FAIL_IF_NO_PEER_CERT, NULL);
   if(ca_file.c_str()!=NULL){
        r=SSL_CTX_load_verify_locations(sslctx_, ca_file.c_str(), NULL /* no CA-directory */);   //The CA-directory paremerters would be added to support multiple CA
        if(!r){
           tls_process_error();
           return;}
   }
   SSL_CTX_set_options(sslctx_, SSL_OP_SINGLE_DH_USE);

  /**Get DN from certificate, and put it into message's attribute */
  
}

MCC_TLS_Client::~MCC_TLS_Client(void) {
   if(sslctx_) SSL_CTX_free(sslctx_);
   if(stream_) delete stream_;
}

MCC_Status MCC_TLS_Client::process(Message& inmsg,Message& outmsg) {
   //  MCC_SOAP_Client ---> MCC_HTTP_Client ---> MCC_TLS_Client ---> MCC_TCP_Client
   // Accepted payload is Raw
   // Returned payload is Stream
   // Extracting payload
   if(!inmsg.Payload()) return MCC_Status();
   if(!stream_) return MCC_Status();
   PayloadRawInterface* inpayload = NULL;
   try {
      inpayload = dynamic_cast<PayloadRawInterface*>(inmsg.Payload());
   } catch(std::exception& e) { };
   if(!inpayload) return MCC_Status();
   // Sending payload
   for(int n=0;;++n) {
      char* buf = inpayload->Buffer(n);
      if(!buf) break;
      int bufsize = inpayload->BufferSize(n);
      if(!(stream_->Put(buf,bufsize))) {
         logger.msg(ERROR, "Failed to send content of buffer.");
         return MCC_Status();
      };
   };
   outmsg.Payload(new PayloadTLSMCC(*stream_, logger));
   return MCC_Status(Arc::STATUS_OK);
}


void MCC_TLS_Client::Next(MCCInterface* next,const std::string& label) {
   if(label.empty()) {
      if(stream_) delete stream_;
      stream_=NULL;
      stream_=new PayloadTLSMCC(next,sslctx_, logger);
   };
   MCC::Next(next,label);
};

