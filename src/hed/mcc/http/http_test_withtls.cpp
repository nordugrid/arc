#include <string>
#include <list>
#include <map>
#include <iostream>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <openssl/crypto.h>
#include <openssl/dh.h>
#include <openssl/dsa.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/opensslv.h>
#include <openssl/pem.h>
#include <openssl/rand.h>
#ifndef NO_RSA
 #include <openssl/rsa.h>
#endif
#include <openssl/ssl.h>
#include <openssl/x509.h>
#include <openssl/x509_vfy.h>

#include "../../mcc/tcp/PayloadTCPSocket.h"
#include "../../mcc/tls/PayloadTLSSocket.h"
#include "PayloadHTTP.h"
#include "../../libs/message/PayloadSOAP.h"


SSL_CTX * sslctx_;

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
static bool tls_random_seed(std::string filename, size_t n)
{
   int r;
   r = RAND_load_file(filename.c_str(), (n > 0 && n < LONG_MAX) ? (long)n : LONG_MAX);
   if (n == 0)
        n = my_MIN_SEED_BYTES;
    if (r < n) {
        std::cerr<<"Error: tls_random_seed from file: could not read files"<<std::endl;
        tls_process_error();
        return false;
    } else {
        tls_rand_seeded_p = 1;
        return true;
    }
}

static bool tls_load_certificate(SSL_CTX* sslctx, std::string cert_file, std::string key_file, std::string password, std::string random_file)
{
  // SSL_CTX_set_default_passwd_cb_userdata(sslctx_,password);
   SSL_CTX_set_default_passwd_cb(sslctx, no_passphrase_callback);  //Now, the authentication is based on no_passphrase credential, it would be modified later to add passphrase support.
   std::cerr<<"Reach here4"<<std::endl;
   if(SSL_CTX_use_certificate_file(sslctx,cert_file.c_str(),SSL_FILETYPE_PEM)!=1&&(SSL_CTX_use_certificate_file(sslctx,cert_file.c_str(),SSL_FILETYPE_ASN1
))!=1)
   {
        std::cerr <<"Error: Can not load certificate file"<<std::endl;
        tls_process_error();
        return false;
   }
   if(SSL_CTX_use_PrivateKey_file(sslctx,key_file.c_str(),SSL_FILETYPE_PEM)!=1&&SSL_CTX_use_PrivateKey_file(sslctx,key_file.c_str(),SSL_FILETYPE_ASN1)!=1)
   {
        std::cerr <<"Error: Can not load key file"<<std::endl;
        tls_process_error();
        return false;
   }
   if(!(SSL_CTX_check_private_key(sslctx)))
   {
        std::cerr <<"Error: Private key does not match certificate"<<std::endl;
        tls_process_error();
        return false;
   }
   if(tls_random_seed(random_file, 0)){return false;}
}

void creattlscontext(void){
  //The following credential filename should be obtained from configuration file later.
   std::string cert_file="cert.pem";
   std::string key_file="key.pem";
   std::string ca_file="ca.pem";
   std::string passwd="";
   int r;
   SSL_load_error_strings();
   if(!SSL_library_init()){
        std::cerr<<"Error: SSL_library_init failed\n"<<std::endl;
        tls_process_error();
        return;
   }
   std::cerr<<"Reach here"<<std::endl;
   /*Initialize the SSL Context object*/
   sslctx_=SSL_CTX_new(SSLv23_client_method());
   if(sslctx_==NULL){
        std::cerr<<"Error: Can not creat the SSL Context object"<<std::endl;
        tls_process_error();
        return;
   }
   std::cerr<<"Reach here1"<<std::endl;
   SSL_CTX_set_mode(sslctx_,SSL_MODE_ENABLE_PARTIAL_WRITE);
   std::cerr<<"Reach here2"<<std::endl;
   
   tls_load_certificate(sslctx_, cert_file, key_file, passwd, key_file);
   std::cerr<<"Reach here3"<<std::endl;
   SSL_CTX_set_verify(sslctx_, SSL_VERIFY_PEER |  SSL_VERIFY_FAIL_IF_NO_PEER_CERT, NULL);
   if(ca_file.c_str()!=NULL){
        r=SSL_CTX_load_verify_locations(sslctx_, ca_file.c_str(), NULL /* no CA-directory */);   //The CA-directory paremerters would be added to support multiple CA
        if(!r){
           tls_process_error();
           return;}
   }
   SSL_CTX_set_options(sslctx_, SSL_OP_SINGLE_DH_USE);

}

void test1(void) {
  std::cout<<std::endl;
  std::cout<<"------- Testing simple file download ------"<<std::endl;
  Arc::PayloadTCPSocket socket("grid.uio.no",80,Arc::Logger::rootLogger);
  Arc::PayloadHTTP request("GET","/index.html",socket);
  if(!request.Flush()) {
    std::cout<<"Failed to send HTTP request"<<std::endl;
    return;
  };
  Arc::PayloadHTTP response(socket);
  std::cout<<"*** RESPONSE ***"<<std::endl;
  for(int n = 0;n<response.Size();++n) std::cout<<response[n];
  std::cout<<std::endl;
}

void test2(void) {
  creattlscontext();
  std::cout<<"------Testing TLS enhanced simple file download ------"<<std::endl;
  Arc::PayloadTCPSocket socket("127.0.0.1",443,Arc::Logger::rootLogger);
  Arc::PayloadTLSSocket tlssocket(socket,sslctx_,true,
				  Arc::Logger::rootLogger);
  Arc::PayloadHTTP request("GET","/index.html",tlssocket);
  if(!request.Flush()) {
    std::cout<<"Failed to send HTTPs request"<<std::endl;
    return;
  };
  Arc::PayloadHTTP response(tlssocket);
  std::cout<<"*** RESPONSE ***"<<std::endl;
  for(int n = 0;n<response.Size();++n) std::cout<<response[n];
  std::cout<<std::endl;
}


int main(void) {
  test2();
  return 0;
}
