#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef WIN32
#include <arc/win32.h>
#endif
#include <stdlib.h>
#include <sys/time.h>

#include <string>
#include <sstream>
#include <fstream>
#include <iostream>

#include <glibmm.h>

#include <xmlsec/base64.h>
#include <xmlsec/errors.h>
#include <xmlsec/xmltree.h>
#include <xmlsec/xmldsig.h>
#include <xmlsec/xmlenc.h>
#include <xmlsec/templates.h>
#include <xmlsec/crypto.h>

#include <xmlsec/openssl/app.h>
#include <openssl/bio.h>

#include <openssl/evp.h>
#include <openssl/sha.h>
#include <openssl/rand.h>
#ifdef CHARSET_EBCDIC
#include <openssl/ebcdic.h>
#endif 

#include "XmlSecUtils.h"

namespace Arc {

int passphrase_callback(char* buf, int size, int rwflag, void *) {
  int len;
  char prompt[128];
  snprintf(prompt, sizeof(prompt), "Enter passphrase for the key file: \n");
  int r = EVP_read_pw_string(buf, size, prompt, 0);
  if(r != 0) {
    std::cerr<<"Failed to read passphrase from stdin"<<std::endl;
    return -1;
  }
  len = strlen(buf);
  if(buf[len-1] == '\n') {
    buf[len-1] = '\0';
    len--;
  }
  return len;
}

class ThreadInitializer {
public:
  ThreadInitializer(void) {
    Glib::init();
    if(!Glib::thread_supported()) Glib::thread_init();
  };
};
static ThreadInitializer thread_initializer;

static Glib::Mutex init_lock_;
static bool has_init = false;

bool init_xmlsec(void) {
  if(!has_init) {
    init_lock_.lock();
    has_init = true;
    init_lock_.unlock();

    //Init libxml and libxslt libraries
    xmlInitParser();

    //Init xmlsec library
    if(xmlSecInit() < 0) {
      std::cerr<<"XMLSec initialization failed"<<std::endl;
      goto err;
    }

    /* Load default crypto engine if we are supporting dynamic
     * loading for xmlsec-crypto libraries. Use the crypto library
     * name ("openssl", "nss", etc.) to load corresponding
     * xmlsec-crypto library.
     */
#ifdef XMLSEC_CRYPTO_DYNAMIC_LOADING
    if(xmlSecCryptoDLLoadLibrary(BAD_CAST XMLSEC_CRYPTO) < 0) {
      std::cerr<<"Unable to load default xmlsec-crypto library. Make sure"
                        "that you have it installed and check shared libraries path"
                        "(LD_LIBRARY_PATH) envornment variable."<<std::endl;
      goto err;
    }
#endif /* XMLSEC_CRYPTO_DYNAMIC_LOADING */

    // Init crypto library
    if(xmlSecCryptoAppInit(NULL) < 0) {
      std::cerr<<"crypto initialization failed"<<std::endl;
      goto err;
    }

    //Init xmlsec-crypto library
    if(xmlSecCryptoInit() < 0) {
      std::cerr<<"xmlsec-crypto initialization failed"<<std::endl;
      goto err;
    }

    return true;

err:
    init_lock_.lock();
    has_init = false;
    init_lock_.unlock();
    return false;
  }
  return true;
}

bool final_xmlsec(void) {
  if(has_init) {
    init_lock_.lock();
    has_init = false;
    init_lock_.unlock();

    //Shutdown xmlsec-crypto library
    xmlSecCryptoShutdown();
    //Shutdown crypto library 
    xmlSecCryptoAppShutdown();  
    //Shutdown xmlsec library
    xmlSecShutdown();
    //Shutdown libxml
    xmlCleanupParser();
  }
}

//Get certificate piece (the string under BEGIN CERTIFICATE : END CERTIFICATE) from a certificate file
std::string get_cert_str(const char* certfile) {
  std::ifstream is(certfile);
  std::string cert;
  std::getline(is,cert, char(0));
  std::size_t pos = cert.find("BEGIN CERTIFICATE");
  if(pos != std::string::npos) {
    std::size_t pos1 = cert.find_first_of("---", pos);
    std::size_t pos2 = cert.find_first_not_of("-", pos1);
    std::size_t pos3 = cert.find_first_of("---", pos2);
    std::string str = cert.substr(pos2+1, pos3-pos2-2);
    return str;
  }
  return ("");
}

xmlSecKey* get_key_from_keyfile(const char* keyfile) {
  std::string key_str;
  std::ifstream is(keyfile);
  std::getline(is,key_str, char(0));

  xmlSecKeyPtr key = NULL;
  key = get_key_from_keystr(key_str);
  return key;
}

//Get key from a binary key 
xmlSecKey* get_key_from_keystr(const std::string& value) {//, const bool usage) { 
  xmlSecKey *key = NULL;
  xmlSecKeyDataFormat key_formats[] = {
    xmlSecKeyDataFormatDer,
    xmlSecKeyDataFormatCertDer,
    xmlSecKeyDataFormatPkcs8Der,
    xmlSecKeyDataFormatCertPem,
    xmlSecKeyDataFormatPkcs8Pem,
    xmlSecKeyDataFormatPem,
    xmlSecKeyDataFormatBinary,
    (xmlSecKeyDataFormat)0
  };

  int rc;
  std::string v(value.size(),'\0');
  xmlSecErrorsDefaultCallbackEnableOutput(FALSE);
  xmlSecByte* tmp_str = new xmlSecByte[value.size()];
  memset(tmp_str,0,value.size());
  rc = xmlSecBase64Decode((const xmlChar*)(value.c_str()), tmp_str, value.size());
  if (rc < 0) {
    //bad base-64
    memcpy(tmp_str,value.c_str(),value.size());
    rc = value.size();
  }
  for (int i=0; key_formats[i] && key == NULL; i++) {
    key = xmlSecCryptoAppKeyLoadMemory(tmp_str, rc, key_formats[i], NULL, NULL, NULL);
  }
  delete[] tmp_str;
  xmlSecErrorsDefaultCallbackEnableOutput(TRUE);

  return key;
}

//Get key from a cert file, return string
std::string get_key_from_certfile(const char* certfile) {
  BIO* certbio = NULL;
  certbio = BIO_new_file(certfile, "r");
  X509* cert = NULL;
  cert = PEM_read_bio_X509(certbio, NULL, NULL, NULL); 
  EVP_PKEY* key = NULL;
  key = X509_get_pubkey(cert);

  BIO* out = NULL;
  out = BIO_new(BIO_s_mem());
  PEM_write_bio_PUBKEY(out, key);

  std::string pubkey_str;
  for(;;) {
    char s[256];
    int l = BIO_read(out,s,sizeof(s));
    if(l <= 0) break;
    pubkey_str.append(s,l);;
  }

  EVP_PKEY_free(key);
  X509_free(cert);
  BIO_free_all(certbio);
  BIO_free_all(out);

  if(!pubkey_str.empty()) {
    std::size_t pos = pubkey_str.find("BEGIN PUBLIC KEY");
    if(pos != std::string::npos) {
      std::size_t pos1 = pubkey_str.find_first_of("---", pos);
      std::size_t pos2 = pubkey_str.find_first_not_of("-", pos1);
      std::size_t pos3 = pubkey_str.find_first_of("---", pos2);
      std::string str = pubkey_str.substr(pos2+1, pos3-pos2-2);
      return str;
    }
    return ("");
  }
  return pubkey_str;
}

//Get key from a cert string
xmlSecKey* get_key_from_certstr(const std::string& value) {
  xmlSecKey *key = NULL;
  xmlSecKeyDataFormat key_formats[] = {
    xmlSecKeyDataFormatDer,
    xmlSecKeyDataFormatCertDer,
    xmlSecKeyDataFormatPkcs8Der,
    xmlSecKeyDataFormatCertPem,
    xmlSecKeyDataFormatPkcs8Pem,
    xmlSecKeyDataFormatPem,
    xmlSecKeyDataFormatBinary,
    (xmlSecKeyDataFormat)0
  };

  int rc;
  xmlSecErrorsDefaultCallbackEnableOutput(FALSE);

  BIO* certbio = NULL;
  std::string cert_value;
  //Here need to compose a complete certificate
  cert_value.append("-----BEGIN CERTIFICATE-----").append("\n").append(value).append("\n").append("-----END CERTIFICATE-----");

  for (int i=0; key_formats[i] && key == NULL; i++) {
    certbio = BIO_new_mem_buf((void*)(cert_value.c_str()), cert_value.size());
    key = xmlSecOpenSSLAppKeyFromCertLoadBIO(certbio, key_formats[i]);
    BIO_free(certbio);
  }

  xmlSecErrorsDefaultCallbackEnableOutput(TRUE);

  return key;
}

//Load private or public key from a key file into key manager
xmlSecKeysMngrPtr load_key_from_keyfile(xmlSecKeysMngrPtr* keys_manager, const char* keyfile) {
  xmlSecKeysMngrPtr keys_mngr;
  if((keys_manager != NULL) && (*keys_manager != NULL)) keys_mngr = *keys_manager;
  else {
    keys_mngr = xmlSecKeysMngrCreate();
    //initialize keys manager
    if (xmlSecCryptoAppDefaultKeysMngrInit(keys_mngr)<0) {
      std::cerr<<"Can not initialize xmlSecKeysMngr object"<<std::endl;
      xmlSecKeysMngrDestroy(keys_mngr); return NULL;
    }
  }
  if(keys_mngr == NULL) { std::cerr<<"Can not create xmlSecKeysMngr object"<<std::endl; return NULL;}

  std::string key_str;
  std::ifstream is(keyfile);
  std::getline(is,key_str, char(0));

  xmlSecKeyPtr key = get_key_from_keystr(key_str);

  if(xmlSecCryptoAppDefaultKeysMngrAdoptKey(keys_mngr, key) < 0) {
    std::cerr<<"Failed to add key from "<<keyfile<<" to keys manager"<<std::endl;
    xmlSecKeyDestroy(key);
    xmlSecKeysMngrDestroy(keys_mngr);
    return NULL;
  }
  if(keys_manager != NULL) keys_manager = &keys_mngr;
  return keys_mngr;
}

//Load public key from a certificate file into key manager
xmlSecKeysMngrPtr load_key_from_certfile(xmlSecKeysMngrPtr* keys_manager, const char* certfile) {
  xmlSecKeysMngrPtr keys_mngr;
  if((keys_manager != NULL) && (*keys_manager != NULL)) keys_mngr = *keys_manager;
  else {
    keys_mngr = xmlSecKeysMngrCreate();
    //initialize keys manager
    if (xmlSecCryptoAppDefaultKeysMngrInit(keys_mngr)<0) {
      std::cerr<<"Can not initialize xmlSecKeysMngr object"<<std::endl;
      xmlSecKeysMngrDestroy(keys_mngr); return NULL;
    }
  }
  if(keys_mngr == NULL) { std::cerr<<"Can not create xmlSecKeysMngr object"<<std::endl; return NULL;}

  std::string cert_str;
  cert_str = get_cert_str(certfile);
  xmlSecKeyPtr key = get_key_from_certstr(cert_str);

  if(xmlSecCryptoAppDefaultKeysMngrAdoptKey(keys_mngr, key) < 0) {
    std::cerr<<"Failed to add key from "<<certfile<<" to keys manager"<<std::endl;
    xmlSecKeyDestroy(key);
    xmlSecKeysMngrDestroy(keys_mngr);
    return NULL;
  }
  if(keys_manager != NULL) keys_manager = &keys_mngr;
  return keys_mngr;
}

//Load public key from a certificate string into key manager
xmlSecKeysMngrPtr load_key_from_certstr(xmlSecKeysMngrPtr* keys_manager, const std::string& certstr) {
  xmlSecKeysMngrPtr keys_mngr;
  if((keys_manager != NULL) && (*keys_manager != NULL)) keys_mngr = *keys_manager;
  else {
    keys_mngr = xmlSecKeysMngrCreate();
    //initialize keys manager
    if (xmlSecCryptoAppDefaultKeysMngrInit(keys_mngr)<0) {
      std::cerr<<"Can not initialize xmlSecKeysMngr object"<<std::endl;
      xmlSecKeysMngrDestroy(keys_mngr); return NULL;
    }
  }
  if(keys_mngr == NULL) { std::cerr<<"Can not create xmlSecKeysMngr object"<<std::endl; return NULL;}

  xmlSecKeyPtr key = get_key_from_certstr(certstr);
  if(xmlSecCryptoAppDefaultKeysMngrAdoptKey(keys_mngr, key) < 0) {
    std::cerr<<"Failed to add key from "<<certstr<<" to keys manager"<<std::endl;
    xmlSecKeyDestroy(key);
    xmlSecKeysMngrDestroy(keys_mngr);
    return NULL;
  }
  if(keys_manager != NULL) keys_manager = &keys_mngr;
  return keys_mngr;
}


//Load trusted certificate from file
xmlSecKeysMngrPtr load_trusted_cert_file(xmlSecKeysMngrPtr* keys_manager, const char* cert_file) {
  xmlSecKeysMngrPtr keys_mngr;
  if((keys_manager != NULL) && (*keys_manager != NULL)) keys_mngr = *keys_manager;
  else {
    keys_mngr = xmlSecKeysMngrCreate();
    //initialize keys manager
    if (xmlSecCryptoAppDefaultKeysMngrInit(keys_mngr)<0) {
      std::cerr<<"Can not initialize xmlSecKeysMngr object"<<std::endl;
      xmlSecKeysMngrDestroy(keys_mngr); return NULL;
    }
  }
  if(keys_mngr == NULL) { std::cerr<<"Can not create xmlSecKeysMngr object"<<std::endl; return NULL;}
  //load cert from file
  if(cert_file && (strlen(cert_file) != 0))
    if(xmlSecCryptoAppKeysMngrCertLoad(keys_mngr, cert_file, xmlSecKeyDataFormatPem, xmlSecKeyDataTypeTrusted) < 0) {
      xmlSecKeysMngrDestroy(keys_mngr);
      return NULL;
    }
  if(keys_manager != NULL) keys_manager = &keys_mngr;
  return keys_mngr;
}

//Could be used for many trusted certificates in string
xmlSecKeysMngrPtr load_trusted_cert_str(xmlSecKeysMngrPtr* keys_manager, const std::string& cert_str) {
  xmlSecKeysMngrPtr keys_mngr;
  if((keys_manager != NULL) && (*keys_manager != NULL)) keys_mngr = *keys_manager;
  else {
    keys_mngr = xmlSecKeysMngrCreate();
    //initialize keys manager
    if (xmlSecCryptoAppDefaultKeysMngrInit(keys_mngr)<0) {
      std::cerr<<"Can not initialize xmlSecKeysMngr object"<<std::endl;
      xmlSecKeysMngrDestroy(keys_mngr); return NULL;
    }
  }
  if(keys_mngr == NULL) { std::cerr<<"Can not create xmlSecKeysMngr object"<<std::endl; return NULL;}

  //load cert from memory
  if(!cert_str.empty())
    if(xmlSecCryptoAppKeysMngrCertLoadMemory(keys_mngr, (const xmlSecByte*)(cert_str.c_str()), 
          (xmlSecSize)(cert_str.size()), xmlSecKeyDataFormatPem, xmlSecKeyDataTypeTrusted) < 0) {
      xmlSecKeysMngrDestroy(keys_mngr);
      return NULL;
    }
  if(keys_manager != NULL) keys_manager = &keys_mngr;
  return keys_mngr;
}

//Load trusted cetificates into key manager
xmlSecKeysMngrPtr load_trusted_certs(xmlSecKeysMngrPtr* keys_manager, const char* cafile, const char* capath) {
  xmlSecKeysMngrPtr keys_mngr;
  if((keys_manager != NULL) && (*keys_manager != NULL)) keys_mngr = *keys_manager;
  else {
    keys_mngr = xmlSecKeysMngrCreate();
    //initialize keys manager
    if (xmlSecCryptoAppDefaultKeysMngrInit(keys_mngr)<0) {
      std::cerr<<"Can not initialize xmlSecKeysMngr object"<<std::endl;
      xmlSecKeysMngrDestroy(keys_mngr); return NULL;
    }
  }
  if(keys_mngr == NULL) { std::cerr<<"Can not create xmlSecKeysMngr object"<<std::endl; return NULL;}

  //load ca certs into keys manager, the two method used here could not work in some old xmlsec verion,
  //because of some bug about X509_FILETYPE_DEFAULT and X509_FILETYPE_PEM 
  //load a ca path
  if(capath && (strlen(capath) != 0))
    if(xmlSecOpenSSLAppKeysMngrAddCertsPath(keys_mngr, capath) < 0) {
      xmlSecKeysMngrDestroy(keys_mngr);
      return NULL;
    }
#if 0
  //load a ca file  TODO: can only be used in some new version of xmlsec
  if(cafile && (strlen(cafile) != 0))  
    if(xmlSecOpenSSLAppKeysMngrAddCertsFile(keys_mngr, cafile) < 0) {
      xmlSecKeysMngrDestroy(keys_mngr);
      return NULL;
  }
#endif
  if(cafile && (strlen(cafile) != 0))
    if(xmlSecCryptoAppKeysMngrCertLoad(keys_mngr, cafile, xmlSecKeyDataFormatPem, xmlSecKeyDataTypeTrusted) < 0) {
      xmlSecKeysMngrDestroy(keys_mngr);
      return NULL;
    }

  if(keys_manager != NULL) keys_manager = &keys_mngr;
  return keys_mngr;
} 

XMLNode get_node(XMLNode& parent,const char* name) {
  XMLNode n = parent[name];
  if(!n) n=parent.NewChild(name);
  return n;
}
} // namespace Arc

