#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "Credential.h"

int main(void) {
  
  //Request side
  BIO* req;
  req = BIO_new(BIO_s_mem());
  Credential request();
  cred.GenerateRequest(req);

  //Signing side
  BIO* out; 
  out = BIO_new(BIO_s_mem());
  Credential proxy();
  Credential signer(cert, key, capath); 
  proxy.InquireRequest(req);
  signer.SignRequest(&proxy, out);

}
