#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <cstring>
#include <ctime>

#include <unistd.h>
#include <sys/types.h>

#include <openssl/pem.h>
#include <openssl/asn1.h>
#include <openssl/x509.h>
#include <openssl/evp.h>
#include <openssl/bio.h>

#ifndef HAVE_TIMEGM
time_t timegm (struct tm *tm) {
  char *tz = getenv("TZ");
#ifdef HAVE_SETENV
  setenv("TZ", "", 1);
#else
  putenv("TZ=");
#endif
  tzset();
  time_t ret = mktime(tm);
  if(tz) {
#ifdef HAVE_SETENV
    setenv("TZ", tz, 1);
#else
    static std::string oldtz;
    oldtz = std::string("TZ=") + tz;
    putenv(strdup(const_cast<char*>(oldtz.c_str())));
#endif
  }
  else {
#ifdef HAVE_UNSETENV
    unsetenv("TZ");
#else
    putenv("TZ");
#endif
  }
  tzset();
  return ret;
}
#endif

#include "OpenSSLFunctions.h"

namespace Arc {

namespace Cream {

time_t ASN1_UTCTIME_get(const ASN1_UTCTIME *s){
  struct tm tm;
  int offset;
  memset(&tm,'\0',sizeof tm);
#define g2(p) (((p)[0]-'0')*10+(p)[1]-'0')
  tm.tm_year=g2(s->data);
  if(tm.tm_year < 50)
    tm.tm_year+=100;
  tm.tm_mon=g2(s->data+2)-1;
  tm.tm_mday=g2(s->data+4);
  tm.tm_hour=g2(s->data+6);
  tm.tm_min=g2(s->data+8);
  tm.tm_sec=g2(s->data+10);
  if(s->data[12] == 'Z')
    offset=0;
  else{
    offset=g2(s->data+13)*60+g2(s->data+15);
    if(s->data[12] == '-')
      offset= -offset;
  }
#undef g2
  return timegm(&tm)-offset*60;
}

const long getCertTimeLeft(const std::string& pxfile) {
  time_t timeleft = 0;
  BIO *in = NULL;
  X509 *x = NULL;
  in = BIO_new(BIO_s_file());
  if (in) {
    BIO_set_close(in, BIO_CLOSE);
    if (BIO_read_filename( in, pxfile.c_str() ) > 0) {
      x = PEM_read_bio_X509(in, NULL, 0, NULL);
      if (x) {
        timeleft = ( ASN1_UTCTIME_get( X509_get_notAfter(x))  - time(NULL) ) / 60 ;
      } else{
        std::cerr << "unable to read X509 proxy file: " << pxfile ;
      }
    } else {
      std::cerr << "unable to open X509 proxy file: " << pxfile;
    }
    BIO_free(in);
    free(x);
  } else {
    std::cerr << "unable to allocate memory for the proxy file: " <<  pxfile ;
  }
  return timeleft;
}

time_t GRSTasn1TimeToTimeT(char *asn1time, size_t len)
{
   char   zone;
   struct tm time_tm;
   
   if (len == 0) len = strlen(asn1time);
                                                                                
   if ((len != 13) && (len != 15)) return 0;
                                                                                
   if ((len == 13) &&
       ((sscanf(asn1time, "%02d%02d%02d%02d%02d%02d%c",
         &(time_tm.tm_year),
         &(time_tm.tm_mon),
         &(time_tm.tm_mday),
         &(time_tm.tm_hour),
         &(time_tm.tm_min),
         &(time_tm.tm_sec),
         &zone) != 7) || (zone != 'Z'))) return 0; 
                                                                                
   if ((len == 15) &&
       ((sscanf(asn1time, "20%02d%02d%02d%02d%02d%02d%c",
         &(time_tm.tm_year),
         &(time_tm.tm_mon),
         &(time_tm.tm_mday),
         &(time_tm.tm_hour),
         &(time_tm.tm_min),
         &(time_tm.tm_sec),
         &zone) != 7) || (zone != 'Z'))) return 0; 
                                                                                
   /* time format fixups */
                                                                                
   if (time_tm.tm_year < 90) time_tm.tm_year += 100;
   --(time_tm.tm_mon);
                                                                                
   return timegm(&time_tm);
}

//modified version of gridsite makeproxy. 
//must be cleaned up.
int makeProxyCert(char **proxychain, char *reqtxt, char *cert, char *key, int minutes)
{
  char *ptr, *certchain;
  int i, ncerts;
  long serial = 1234, ptrlen;
  EVP_PKEY *pkey, *CApkey;
  const EVP_MD *digest;
  X509 *certs[9];
  X509_REQ *req;
  X509_NAME *name, *CAsubject, *newsubject;
  X509_NAME_ENTRY *ent;
  FILE *fp;
  BIO *reqmem, *certmem;
  time_t notAfter;

  /* read in the request */
  reqmem = BIO_new(BIO_s_mem());
  BIO_puts(reqmem, reqtxt);
    
  if (!(req = PEM_read_bio_X509_REQ(reqmem, NULL, NULL, NULL)))
    {
      std::cerr << "MakeProxyCert(): error reading request from BIO memory\n";
      BIO_free(reqmem);
      return -1;
    }
    
  BIO_free(reqmem);

  /* verify signature on the request */
  if (!(pkey = X509_REQ_get_pubkey(req)))
    {
      std::cerr << "MakeProxyCert(): error getting public key from request\n";
      
      X509_REQ_free(req);
      return -1;
    }

  if (X509_REQ_verify(req, pkey) != 1)
    {
      std::cerr << "MakeProxyCert(): error verifying signature on certificate\n";

      X509_REQ_free(req);
      return -1;
    }
    
  /* read in the signing certificate */
  if (!(fp = fopen(cert, "r")))
    {
      std::cerr << "MakeProxyCert(): error opening signing certificate file\n";

      X509_REQ_free(req);
      return -1;
    }    

  for (ncerts = 1; ncerts < 9; ++ncerts)
   if ((certs[ncerts] = PEM_read_X509(fp, NULL, NULL, NULL)) == NULL) break;

  if (ncerts == 1) /* zeroth cert with be new proxy cert */
    {
      std::cerr << "MakeProxyCert(): error reading signing certificate file\n";

      X509_REQ_free(req);
      return -1;
    }    

  fclose(fp);
  
  CAsubject = X509_get_subject_name(certs[1]);

  /* read in the CA private key */
  if (!(fp = fopen(key, "r")))
    {
      std::cerr << "MakeProxyCert(): error reading signing private key file\n";

      X509_REQ_free(req);
      return -1;
    }    

  if (!(CApkey = PEM_read_PrivateKey(fp, NULL, NULL, NULL)))
    {
      std::cerr << "MakeProxyCert(): error reading signing private key in file\n";

      X509_REQ_free(req);
      return -1;
    }    

  fclose(fp);
  
  /* get subject name */
  if (!(name = X509_REQ_get_subject_name(req)))
    {
      std::cerr << "MakeProxyCert(): error getting subject name from request\n";

      X509_REQ_free(req);
      return -1;
    }    

  /* create new certificate */
  if (!(certs[0] = X509_new()))
    {
      std::cerr << "MakeProxyCert(): error creating X509 object\n";

      X509_REQ_free(req);
      return -1;
    }    

  /* set version number for the certificate (X509v3) and the serial number   
     need 3 = v4 for GSI proxy?? */
  if (X509_set_version(certs[0], 3L) != 1)
    {
      std::cerr << "MakeProxyCert(): error setting certificate version\n";

      X509_REQ_free(req);
      return -1;
    }    

  ASN1_INTEGER_set(X509_get_serialNumber(certs[0]), serial++);

  if (!(name = X509_get_subject_name(certs[1])))
    {
      std::cerr << "MakeProxyCert(): error getting subject name from CA certificate\n";

      X509_REQ_free(req);
      return -1;
    }    

  if (X509_set_issuer_name(certs[0], name) != 1)
    {
     std::cerr << "MakeProxyCert(): error setting issuer name of certificate\n";

      X509_REQ_free(req);
      return -1;
    }    

  /* set issuer and subject name of the cert from the req and the CA */
  ent = X509_NAME_ENTRY_create_by_NID(NULL, OBJ_txt2nid("commonName"), MBSTRING_ASC, (unsigned char*) "proxy", -1);

  newsubject = X509_NAME_dup(CAsubject);

  X509_NAME_add_entry(newsubject, ent, -1, 0);

  if (X509_set_subject_name(certs[0], newsubject) != 1)
    {
      std::cerr << "MakeProxyCert(): error setting subject name of certificate\n";

      X509_REQ_free(req);
      return -1;
    }    
    
  X509_NAME_free(newsubject);
  X509_NAME_ENTRY_free(ent);

  /* set public key in the certificate */
  if (X509_set_pubkey(certs[0], pkey) != 1)
    {
      std::cerr << "MakeProxyCert(): error setting public key of the certificate\n";

      X509_REQ_free(req);
      return -1;
    }    

  /* set duration for the certificate */
  if (!(X509_gmtime_adj(X509_get_notBefore(certs[0]), -300)))
    {
      std::cerr << "MakeProxyCert(): error setting beginning time of the certificate\n";

      X509_REQ_free(req);
      return -1;
    }    

  if (!(X509_gmtime_adj(X509_get_notAfter(certs[0]), 60 * minutes)))
    {
      std::cerr << "MakeProxyCert(): error setting ending time of the certificate\n";

      X509_REQ_free(req);
      return -1;
    }
    
  /* go through chain making sure this proxy is not longer lived */

  notAfter = GRSTasn1TimeToTimeT((char*) ASN1_STRING_data(X509_get_notAfter(certs[0])), 0);

  for (i=1; i < ncerts; ++i)
       if (notAfter > GRSTasn1TimeToTimeT((char*)ASN1_STRING_data(X509_get_notAfter(certs[i])),0))
         {
           notAfter = GRSTasn1TimeToTimeT((char*) ASN1_STRING_data(X509_get_notAfter(certs[i])),0);
            
           ASN1_UTCTIME_set(X509_get_notAfter(certs[0]), notAfter);
         }

  /* sign the certificate with the signing private key */
  if (EVP_PKEY_type(CApkey->type) == EVP_PKEY_RSA)
    digest = EVP_md5();
  else
    {
      std::cerr << "MakeProxyCert(): error checking signing private key for a valid digest\n";

      X509_REQ_free(req);
      return -1;
    }    

  if (!(X509_sign(certs[0], CApkey, digest)))
    {
      std::cerr << "MakeProxyCert(): error signing certificate\n";

      X509_REQ_free(req);
      return -1;
    }    

  /* store the completed certificate chain */

  certchain = strdup("");

  for (i=0; i < ncerts; ++i)
     {
       certmem = BIO_new(BIO_s_mem());

       if (PEM_write_bio_X509(certmem, certs[i]) != 1)
         {
           std::cerr <<  "MakeProxyCert(): error writing certificate to memory BIO\n";            

           X509_REQ_free(req);
           return -1;
         }

       ptrlen = BIO_get_mem_data(certmem, &ptr);
  
       certchain =(char*) realloc(certchain, strlen(certchain) + ptrlen + 1);
       
       strncat(certchain, ptr, ptrlen);
    
       BIO_free(certmem);
       X509_free(certs[i]);
     }
  
  EVP_PKEY_free(pkey);
  EVP_PKEY_free(CApkey);
  X509_REQ_free(req);
      
  *proxychain = certchain;  
  return 0;
}


std::string checkPath(std::string p){
   std::ifstream inf(p.c_str());
   if (inf.good()){
      inf.close();
      return p;
   }
   inf.close();
   return p.assign("");
} 

std::string getProxy(){
   std::string path;
   char * res=NULL;

   res = getenv ("X509_USER_PROXY");
   if ( res!=NULL ){
      path = checkPath(res);
   }else{
      std::stringstream uid;
      uid << "/tmp/x509up_u" << getuid() ;
      path = checkPath(uid.str()); 
   }
   return path;
}

std::string getTrustedCerts(){
   std::string path;
   char * res=NULL;

   res = getenv ("X509_CERT_DIR");
   if ( !res ){
      path = checkPath("/etc/grid-security/certificates/");
   }else{
      path.assign(res); 
   }
   return path;
}

} // namespace Cream

} // namespace Arc
