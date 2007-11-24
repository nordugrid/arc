#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <sys/types.h>
#ifndef WIN32
#include <sys/socket.h>
#include <arpa/inet.h>
#else
#define NOGDI
#endif
#include <errno.h>

#include <arc/message/PayloadStream.h>
#include <arc/message/PayloadRaw.h>
#include <arc/loader/Loader.h>
#include <arc/loader/MCCLoader.h>
#include <arc/XMLNode.h>

#include <openssl/err.h>
#include <openssl/rand.h>
#include <openssl/ssl.h>

#include "GlobusSigningPolicy.h"

#include "PayloadTLSStream.h"
#include "PayloadTLSMCC.h"

#include "MCCTLS.h"

bool Arc::MCC_TLS::ssl_initialized_ = false;
Glib::Mutex Arc::MCC_TLS::lock_;
Glib::Mutex* Arc::MCC_TLS::ssl_locks_ = NULL;
Arc::Logger Arc::MCC_TLS::logger(Arc::MCC::logger,"TLS");

static int ex_data_index_ = -1;

static void store_MCC_TLS(SSL_CTX* container,Arc::MCC_TLS* it) {
  if(ex_data_index_ != -1) {
    SSL_CTX_set_ex_data(container,ex_data_index_,it);
    return;
  };
  Arc::Logger::getRootLogger().msg(Arc::ERROR,"Failed to store application data into OpenSSL");
}

static Arc::MCC_TLS* retrieve_MCC_TLS(X509_STORE_CTX* container) {
  Arc::MCC_TLS* it = NULL;
  if(ex_data_index_ != -1) {
    SSL* ssl = (SSL*)X509_STORE_CTX_get_ex_data(container,SSL_get_ex_data_X509_STORE_CTX_idx());
    if(ssl != NULL) {
      SSL_CTX* ssl_ctx = SSL_get_SSL_CTX(ssl);
      if(ssl_ctx != NULL) {
        it = (Arc::MCC_TLS*)SSL_CTX_get_ex_data(ssl_ctx,ex_data_index_);
      }
    };
  };
  if(it == NULL) {
    Arc::Logger::getRootLogger().msg(Arc::ERROR,"Failed to retireve application data from OpenSSL");
  };
  return it;
}


static int verify_callback(int ok,X509_STORE_CTX *sctx) {
#ifndef HAVE_OPENSSL_X509_VERIFY_PARAM
   // Not sure if this will work
   if(!(sctx->flags & X509_V_FLAG_CRL_CHECK)) {
     sctx->flags |= X509_V_FLAG_CRL_CHECK;
     ok=X509_verify_cert(sctx);
     return ok;
   };
#endif
  if (ok != 1) {
    int err = X509_STORE_CTX_get_error(sctx);
    switch(err) {
#ifdef HAVE_OPENSSL_PROXY
      case X509_V_ERR_PROXY_CERTIFICATES_NOT_ALLOWED: {
        // This shouldn't happen here because flags are already set
        // to allow proxy. But one can never know because used flag 
        // setting method is undocumented.
        X509_STORE_CTX_set_flags(sctx,X509_V_FLAG_ALLOW_PROXY_CERTS);
        // Calling X509_verify_cert will cause a recursive call to verify_callback.
        // But there should be no loop because PROXY_CERTIFICATES_NOT_ALLOWED error 
        // can't happen anymore.
        ok=X509_verify_cert(sctx);
        if(ok == 1) X509_STORE_CTX_set_error(sctx,X509_V_OK);
      }; break;
#endif
      case X509_V_ERR_UNABLE_TO_GET_CRL: {
        // Missing CRL is not an error (TODO: make it configurable)
        // Consider that to be a policy of site like Globus does
        if(sctx->param) {
          // Not sure of there is need for recursive X509_verify_cert() here.
          // It looks like openssl runs tests sequentially for whole chain.
          // But not sure if behavior is same in all versions.
#ifdef HAVE_OPENSSL_X509_VERIFY_PARAM
          X509_VERIFY_PARAM_clear_flags(sctx->param,X509_V_FLAG_CRL_CHECK);
          ok=X509_verify_cert(sctx);
          X509_VERIFY_PARAM_set_flags(sctx->param,X509_V_FLAG_CRL_CHECK);
#else
          sctx->flags &= ~X509_V_FLAG_CRL_CHECK;
          ok=X509_verify_cert(sctx);
          sctx->flags |= X509_V_FLAG_CRL_CHECK;
#endif
          if(ok == 1) X509_STORE_CTX_set_error(sctx,X509_V_OK);
        };
      }; break;
    };
  };
  if(ok == 1) {
    // Do additional verification here.
    Arc::MCC_TLS* it = retrieve_MCC_TLS(sctx);
    if(it != NULL) {
      // Globus signing policy
      // Do not apply to proxies and self-signed CAs.
      if((it->GlobusPolicy()) && (!(it->CADir().empty()))) {
        X509* cert = X509_STORE_CTX_get_current_cert(sctx);
#ifdef HAVE_OPENSSL_PROXY
        int pos = X509_get_ext_by_NID(cert,NID_proxyCertInfo,-1);
        if(pos < 0) {
#else
        {
#endif
          std::istream* in = Arc::open_globus_policy(X509_get_issuer_name(cert),it->CADir());
          if(in) {
            if(!Arc::match_globus_policy(*in,X509_get_issuer_name(cert),X509_get_subject_name(cert))) {
              char* s = X509_NAME_oneline(X509_get_subject_name(cert),NULL,0);
              Arc::Logger::getRootLogger().msg(Arc::ERROR,"Certificate %s failed Globus signing policy",s);
              OPENSSL_free(s);
              ok=0;
              X509_STORE_CTX_set_error(sctx,X509_V_ERR_SUBJECT_ISSUER_MISMATCH);
            };
            delete in;
          };
        };
      };
    };
  };
  return ok;
}

Arc::MCC_TLS::MCC_TLS(Arc::Config *cfg) : MCC(cfg), globus_policy_(false) {
}

static Arc::MCC* get_mcc_service(Arc::Config *cfg,Arc::ChainContext*) {
    return new Arc::MCC_TLS_Service(cfg);
}

static Arc::MCC* get_mcc_client(Arc::Config *cfg,Arc::ChainContext*) {
    return new Arc::MCC_TLS_Client(cfg);
}


mcc_descriptors ARC_MCC_LOADER = {
    { "tls.service", 0, &get_mcc_service },
    { "tls.client", 0, &get_mcc_client },
    { NULL, 0, NULL }
};

using namespace Arc;


static void tls_process_error(Logger& logger){
   unsigned long err;
   err = ERR_get_error();
   if (err != 0)
   {
     logger.msg(ERROR, "OpenSSL Error -- %s", ERR_error_string(err, NULL));
     logger.msg(ERROR, "Library  : %s", ERR_lib_error_string(err));
     logger.msg(ERROR, "Function : %s", ERR_func_error_string(err));
     logger.msg(ERROR, "Reason   : %s", ERR_reason_error_string(err));
   }
   return;
}

static int no_passphrase_callback(char*, int, int, void*) {
   return -1;
}

/**Input the passphrase for key file
static int passphrase_callback(char* buf, int size, int rwflag, void *) {
   int len;
   std::cout<<"Enter passphrase for your key file:"<<std::endl;
   if(!(fgets(buf, size, stdin))) {
     std::cerr<< "Failed to read passphrase from stdin"<<std::endl;
     return -1;
   }
   len = strlen(buf);
   if(buf[len-1] == '\n') {
     buf[len-1] = '\0';
     len--;
   }
   return len;
}
*/

static int tls_rand_seeded_p = 0;
#define my_MIN_SEED_BYTES 256 
static bool tls_random_seed(Logger& logger, std::string filename, long n)
{
   int r;
   r = RAND_load_file(filename.c_str(), (n > 0 && n < LONG_MAX) ? n : LONG_MAX);
   if (n == 0)
	n = my_MIN_SEED_BYTES;
    if (r < n) {
        logger.msg(ERROR, "tls_random_seed from file: could not read files");
   	tls_process_error(logger);
	return false;
    } else {
	tls_rand_seeded_p = 1;
	return true;
    }
}

static DH *tls_dhe1024 = NULL; /* generating these takes a while, so do it just once */
static void tls_set_dhe1024(Logger& logger)
{
   int i;
   RAND_bytes((unsigned char *) &i, sizeof i);
   if (i < 0)
       i = -i;
   DSA *dsaparams;
   DH *dhparams;
   const char *seed[] = { "2007-10-12: KnowARC ",
                          "makes available the ",
                          "first technology pre",
                          "view of the next gen",
                          "eration ARC1 middlew",
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
	tls_process_error(logger);
	return;
    }
    dhparams = DSA_dup_DH(dsaparams);
    DSA_free(dsaparams);
    if (dhparams == NULL) {
	tls_process_error(logger);
	return;
    }
    if (tls_dhe1024 != NULL)
	DH_free(tls_dhe1024);
    	tls_dhe1024 = dhparams;
}

bool MCC_TLS::tls_load_certificate(SSL_CTX* sslctx, const std::string& cert_file, const std::string& key_file, const std::string&, const std::string& random_file)
{
   // SSL_CTX_set_default_passwd_cb_userdata(sslctx_,password.c_str());
   SSL_CTX_set_default_passwd_cb(sslctx, no_passphrase_callback);
   //SSL_CTX_set_default_passwd_cb(sslctx, passphrase_callback);

   if((SSL_CTX_use_certificate_chain_file(sslctx,cert_file.c_str()) != 1) && 
      (SSL_CTX_use_certificate_file(sslctx,cert_file.c_str(),SSL_FILETYPE_PEM) != 1) && 
      (SSL_CTX_use_certificate_file(sslctx,cert_file.c_str(),SSL_FILETYPE_ASN1) != 1)) {
        logger.msg(ERROR, "Can not load certificate file");
        tls_process_error(logger);
        return false;
   }
   if((SSL_CTX_use_PrivateKey_file(sslctx,key_file.c_str(),SSL_FILETYPE_PEM) != 1) &&
      (SSL_CTX_use_PrivateKey_file(sslctx,key_file.c_str(),SSL_FILETYPE_ASN1) != 1)) {
        logger.msg(ERROR, "Can not load key file");
        tls_process_error(logger);
        return false;
   }
   if(!(SSL_CTX_check_private_key(sslctx))) {
        logger.msg(ERROR, "Private key does not match certificate");
        tls_process_error(logger);
        return false;
   }
   if(tls_random_seed(logger, random_file, 0)) {
     return false;
   }
   return true;
}

void MCC_TLS::ssl_locking_cb(int mode, int n, const char *, int) {
  if(!ssl_locks_) return;
  if(mode & CRYPTO_LOCK) {
    ssl_locks_[n].lock();
  } else {
    ssl_locks_[n].unlock();
  };
}

unsigned long MCC_TLS::ssl_id_cb(void) {
  return (unsigned long)(Glib::Thread::self());
}

//void* MCC_TLS::ssl_idptr_cb(void) {
//  return (void*)(Glib::Thread::self());
//}

bool MCC_TLS::do_ssl_init(void) {
   if(ssl_initialized_) return true;
   lock_.lock();
   if(!ssl_initialized_) {
     logger.msg(INFO, "MCC_TLS::do_ssl_init");     
     SSL_load_error_strings();
     if(!SSL_library_init()){
       logger.msg(ERROR, "SSL_library_init failed");
       tls_process_error(logger);
     } else {
       int num_locks = CRYPTO_num_locks();
       if(num_locks) {
         ssl_locks_=new Glib::Mutex[num_locks];
         if(ssl_locks_) {
           CRYPTO_set_locking_callback(&ssl_locking_cb);
           CRYPTO_set_id_callback(&ssl_id_cb);
           //CRYPTO_set_idptr_callback(&ssl_idptr_cb);
           ssl_initialized_=true;
         } else {
           logger.msg(ERROR, "Failed to allocate SSL locks");
         };
       } else {
         ssl_initialized_=true;
       };
     };
   };
   if(ssl_initialized_) ex_data_index_=SSL_CTX_get_ex_new_index(0,(void*)("MCC_TLS"),NULL,NULL,NULL);
   lock_.unlock();
   return ssl_initialized_;
}

class MCC_TLS_Context:public MessageContextElement {
 public:
  PayloadTLSMCC* stream;
  MCC_TLS_Context(PayloadTLSMCC* s = NULL):stream(s) { };
  virtual ~MCC_TLS_Context(void) { if(stream) delete stream; };
};

/*The main functionality of the constructor method is creat SSL context object*/
MCC_TLS_Service::MCC_TLS_Service(Arc::Config *cfg):MCC_TLS(cfg),sslctx_(NULL) {
   cert_file_ = (std::string)((*cfg)["CertificatePath"]);
   key_file_ = (std::string)((*cfg)["KeyPath"]);
   ca_file_ = (std::string)((*cfg)["CACertificatePath"]);
   ca_dir_ = (std::string)((*cfg)["CACertificatesDir"]);
   globus_policy_ = (((std::string)(*cfg)["CACertificatesDir"].Attribute("PolicyGlobus")) == "true");
   proxy_file_ = (std::string)((*cfg)["ProxyPath"]);
   if(cert_file_.empty()) cert_file_="/etc/grid-security/hostcert.pem";
   if(key_file_.empty()) key_file_="/etc/grid-security/hostkey.pem";
   if(ca_dir_.empty()) ca_dir_="/etc/grid-security/certificates";
   if(!proxy_file_.empty()) { key_file_=proxy_file_; cert_file_=proxy_file_; };
   int r;
   if(!do_ssl_init()) return;
   /*Initialize the SSL Context object*/
   sslctx_=SSL_CTX_new(SSLv23_server_method());
   if(sslctx_==NULL){
        logger.msg(ERROR, "Can not create the SSL Context object");
	tls_process_error(logger);
	return;
   }
   SSL_CTX_set_mode(sslctx_,SSL_MODE_ENABLE_PARTIAL_WRITE);
   tls_load_certificate(sslctx_, cert_file_, key_file_, "", key_file_);
   SSL_CTX_set_verify(sslctx_, SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT | SSL_VERIFY_CLIENT_ONCE, &verify_callback);
   if((!ca_file_.empty()) || (!ca_dir_.empty())){
      r=SSL_CTX_load_verify_locations(sslctx_, ca_file_.empty()?NULL:ca_file_.c_str(), ca_dir_.empty()?NULL:ca_dir_.c_str());
      if(!r){
         tls_process_error(logger);
         SSL_CTX_free(sslctx_); sslctx_=NULL;
         return;
      }   
      /*
      SSL_CTX_set_client_CA_list(sslctx_,SSL_load_client_CA_file(ca_file.c_str())); //Scan all certificates in CAfile and list them as acceptable CAs
      if(SSL_CTX_get_client_CA_list(sslctx_) == NULL){ 
         logger.msg(ERROR, "Can not set client CA list from the specified file");
   	 tls_process_error(logger);
         SSL_CTX_free(sslctx_); sslctx_=NULL;
	 return;
      }
      */
   }
   if(sslctx_->param == NULL) {
     logger.msg(ERROR,"Can't set OpenSSL verify flags");
     SSL_CTX_free(sslctx_); sslctx_=NULL;
     return;
   } else {
#ifdef HAVE_OPENSSL_X509_VERIFY_PARAM
#ifdef HAVE_OPENSSL_PROXY
     if(sslctx_->param) X509_VERIFY_PARAM_set_flags(sslctx_->param,X509_V_FLAG_CRL_CHECK | X509_V_FLAG_ALLOW_PROXY_CERTS);
#else
     if(sslctx_->param) X509_VERIFY_PARAM_set_flags(sslctx_->param,X509_V_FLAG_CRL_CHECK);
#endif
#endif
   };
   store_MCC_TLS(sslctx_,this);
   if(tls_dhe1024 == NULL) { // TODO: Is it needed?
   	tls_set_dhe1024(logger);
	if(tls_dhe1024 == NULL){return;}
   }
   if (!SSL_CTX_set_tmp_dh(sslctx_, tls_dhe1024)) { // TODO: Is it needed?
     logger.msg(ERROR, "DH set error");
     tls_process_error(logger);
     SSL_CTX_free(sslctx_); sslctx_=NULL;
     return;
   }
   SSL_CTX_set_options(sslctx_, SSL_OP_SINGLE_DH_USE | SSL_OP_NO_SSLv2);
#ifndef NO_RSA
   RSA *tmpkey;
   tmpkey = RSA_generate_key(512, RSA_F4, 0, NULL); // TODO: Is it needed?
   if (tmpkey == NULL)
	tls_process_error(logger);
   if (!SSL_CTX_set_tmp_rsa(sslctx_, tmpkey)) {
     RSA_free(tmpkey);
     tls_process_error(logger);
     SSL_CTX_free(sslctx_); sslctx_=NULL;
     return;
   }
   RSA_free(tmpkey);
#endif
}


MCC_TLS_Service::~MCC_TLS_Service(void) {
   if(sslctx_!=NULL)SSL_CTX_free(sslctx_);
}

MCC_Status MCC_TLS_Service::process(Message& inmsg,Message& outmsg) {
   // Accepted payload is StreamInterface 
   // Returned payload is undefined - currently holds no information

   if(!sslctx_) return MCC_Status();

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
      stream = new PayloadTLSMCC(inpayload,sslctx_,logger);
      context=new MCC_TLS_Context(stream);
      inmsg.Context()->Add("tls.service",context);
   };
 
   // Creating message to pass to next MCC
   Message nextinmsg = inmsg;
   nextinmsg.Payload(stream);
   Message nextoutmsg = outmsg; nextoutmsg.Payload(NULL);

   //Getting the subject name of peer(client) certificate
   X509* peercert = NULL;
   char buf[100];     
   peercert = (dynamic_cast<PayloadTLSStream*>(stream))->GetPeercert();
   if (peercert != NULL) {
      X509_NAME_oneline(X509_get_subject_name(peercert),buf,sizeof buf);
      std::string peer_dn = buf;
      logger.msg(DEBUG, "DN name: %s", peer_dn.c_str());
      // Putting the subject name into nextoutmsg.Attribute; so far, the subject is put into Attribute temporally, 
      // it should be put into MessageAuth later.
      nextinmsg.Attributes()->set("TLS:PEERDN",peer_dn);
   }
   
   // Call next MCC 
   MCCInterface* next = Next();
   if(!next)  return MCC_Status();
   MCC_Status ret = next->process(nextinmsg,nextoutmsg);
   // TODO: If next MCC returns anything redirect it to stream
   if(nextoutmsg.Payload()) {
      delete nextoutmsg.Payload();
      nextoutmsg.Payload(NULL);
   };
   if(!ret) return MCC_Status();
   // For nextoutmsg, nothing to do for payload of msg, but 
   // transfer some attributes of msg
   outmsg = nextoutmsg;
   return MCC_Status(Arc::STATUS_OK);
}

MCC_TLS_Client::MCC_TLS_Client(Arc::Config *cfg):MCC_TLS(cfg){
   stream_=NULL;
   cert_file_ = (std::string)((*cfg)["CertificatePath"]);
   key_file_ = (std::string)((*cfg)["KeyPath"]);
   ca_file_ = (std::string)((*cfg)["CACertificatePath"]);
   ca_dir_ = (std::string)((*cfg)["CACertificatesDir"]);
   globus_policy_ = (((std::string)(*cfg)["CACertificatesDir"].Attribute("PolicyGlobus")) == "true");
   proxy_file_ = (std::string)((*cfg)["ProxyPath"]);
   if(cert_file_.empty()) cert_file_="cert.pem";
   if(key_file_.empty()) key_file_="key.pem";
   if(ca_dir_.empty()) ca_dir_="/etc/grid-security/certificates";
   if(!proxy_file_.empty()) { key_file_=proxy_file_; cert_file_=proxy_file_; };
   int r;
   if(!do_ssl_init()) return;
   /*Initialize the SSL Context object*/
   sslctx_=SSL_CTX_new(SSLv23_client_method());
   if(sslctx_==NULL){
        logger.msg(ERROR, "Can not create the SSL Context object");
        tls_process_error(logger);
        return;
   }
   SSL_CTX_set_mode(sslctx_,SSL_MODE_ENABLE_PARTIAL_WRITE);
   tls_load_certificate(sslctx_, cert_file_, key_file_, "", key_file_);
   SSL_CTX_set_verify(sslctx_, SSL_VERIFY_PEER |  SSL_VERIFY_FAIL_IF_NO_PEER_CERT, &verify_callback);
   if((!ca_file_.empty()) || (!ca_dir_.empty())) {
        r=SSL_CTX_load_verify_locations(sslctx_, ca_file_.empty()?NULL:ca_file_.c_str(), ca_dir_.empty()?NULL:ca_dir_.c_str());
        if(!r){
           tls_process_error(logger);
           return;
        }
   }
   SSL_CTX_set_options(sslctx_, SSL_OP_SINGLE_DH_USE);
   if(sslctx_->param == NULL) {
     logger.msg(ERROR,"Can't set OpenSSL verify flags");
     SSL_CTX_free(sslctx_); sslctx_=NULL;
     return;
   } else {
#ifdef HAVE_OPENSSL_X509_VERIFY_PARAM
#ifdef HAVE_OPENSSL_PROXY
     if(sslctx_->param) X509_VERIFY_PARAM_set_flags(sslctx_->param,X509_V_FLAG_CRL_CHECK | X509_V_FLAG_ALLOW_PROXY_CERTS);
#else
     if(sslctx_->param) X509_VERIFY_PARAM_set_flags(sslctx_->param,X509_V_FLAG_CRL_CHECK);
#endif
#endif
   };
   store_MCC_TLS(sslctx_,this);
  /**Get DN from certificate, and put it into message's attribute */
}

MCC_TLS_Client::~MCC_TLS_Client(void) {
   if(sslctx_) SSL_CTX_free(sslctx_);
   if(stream_) delete stream_;
}

MCC_Status MCC_TLS_Client::process(Message& inmsg,Message& outmsg) {
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
         logger.msg(ERROR, "Failed to send content of buffer");
         return MCC_Status();
      };
   };
   outmsg.Payload(new PayloadTLSMCC(*stream_,logger));
   //outmsg.Attributes(inmsg.Attributes());
   //outmsg.Context(inmsg.Context());
   return MCC_Status(Arc::STATUS_OK);
}


void MCC_TLS_Client::Next(MCCInterface* next,const std::string& label) {
   if(label.empty()) {
      if(stream_) delete stream_;
      stream_=NULL;
      stream_=new PayloadTLSMCC(next,sslctx_, logger);
   };
   MCC::Next(next,label);
}
