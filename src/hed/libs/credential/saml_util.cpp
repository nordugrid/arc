#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>
#include <fstream>
#include <glibmm/fileutils.h>
#include <unistd.h>
#include <cstring>
#include <zlib.h>

#include <libxml/uri.h>

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

#include "saml_util.h"

namespace Arc {

  std::string SignQuery(std::string query, SignatureMethod sign_method, std::string& privkey_file) {
 
    std::string ret;

    BIO* key_bio = BIO_new_file(privkey_file.c_str(), "rb");
    if (key_bio == NULL) {
     std::cout<<"Failed to open private key file: "<<privkey_file<<std::endl;
     return ret;
    }

    /* add SigAlg */
    char *t;
    std::string new_query; 
    switch (sign_method) {
      case RSA_SHA1:
        t = (char*)xmlURIEscapeStr(xmlSecHrefRsaSha1, NULL);
        new_query.append(query).append("&SigAlg=").append(t);
	xmlFree(t);
	break;
      case DSA_SHA1:
	t = (char*)xmlURIEscapeStr(xmlSecHrefDsaSha1, NULL);
        new_query.append(query).append("&SigAlg=").append(t);
	xmlFree(t);
	break;
    }

    /* build buffer digest */
    if(new_query.empty()) return ret;
   
    xmlChar* md;
    md = (xmlChar*)(xmlMalloc(20));
    char* digest = (char*)SHA1((unsigned char*)(new_query.c_str()), new_query.size(), md);


    RSA *rsa = NULL;
    DSA *dsa = NULL;
    unsigned char *sigret = NULL;
    unsigned int siglen;
    char *b64_sigret = NULL, *e_b64_sigret = NULL;
    int status = 0;

    /* calculate signature value */
    if (sign_method == RSA_SHA1) {
      rsa = PEM_read_bio_RSAPrivateKey(key_bio, NULL, NULL, NULL);
      if (rsa == NULL) {
        std::cerr<<"Failed to read rsa key from private key file"<<std::endl;
        BIO_free(key_bio); xmlFree(digest); return ret;
      }
      sigret = (unsigned char *)malloc (RSA_size(rsa));
      status = RSA_sign(NID_sha1, (unsigned char*)digest, 20, sigret, &siglen, rsa);
      RSA_free(rsa);
    } 
    else if (sign_method == DSA_SHA1) {
      dsa = PEM_read_bio_DSAPrivateKey(key_bio, NULL, NULL, NULL);
      if (dsa == NULL) {
        std::cerr<<"Failed to read dsa key from private key file"<<std::endl;
        BIO_free(key_bio); xmlFree(digest); return ret;
      }
      sigret = (unsigned char *)malloc (DSA_size(dsa));
      status = DSA_sign(NID_sha1, (unsigned char*)digest, 20, sigret, &siglen, dsa);
      DSA_free(dsa);
    }
    
    BIO_free(key_bio);
   
    if (status ==0) { free(sigret); xmlFree(digest); return ret; }

    /* Base64 encode the signature value */
    b64_sigret = (char*)xmlSecBase64Encode(sigret, siglen, 0);
    /* escape b64_sigret */
    e_b64_sigret = (char*)xmlURIEscapeStr((xmlChar*)b64_sigret, NULL);

    /* add signature */
    switch (sign_method) {
      case RSA_SHA1:
	new_query.append("&Signature=").append(e_b64_sigret);
	break;
      case DSA_SHA1:
        new_query.append("&Signature=").append(e_b64_sigret);
	break;
    }

    xmlFree(digest);
    free(sigret);
    xmlFree(b64_sigret);
    xmlFree(e_b64_sigret);

    return new_query;
  }

  bool VerifyQuery(const std::string query, const xmlSecKey *sender_public_key) {

    /* split query, the signature MUST be the last param of the query,
     * there could be more params in the URL; but they wouldn't be
     * covered by the signature */
    size_t f;
    f = query.find("&Signature=");
    if(f == std::string::npos) { std::cerr<<"Failed to find signature in the query"<<std::endl; return false; }

    std::string str0 = query.substr(0,f-1);
    std::string str1 = query.substr(f+11);

    f = str0.find("&SigAlg=");
    if(f == std::string::npos) { std::cerr<<"Failed to find signature alg in the query"<<std::endl; return false; }

    std::string sig_alg = str0.substr(f+8);

    int key_size;
    RSA *rsa = NULL;
    DSA *dsa = NULL;
    char* usig_alg = NULL;
    usig_alg = xmlURIUnescapeString(sig_alg.c_str(), 0, NULL);
    if (strcmp(usig_alg, (char*)xmlSecHrefRsaSha1) == 0) {
      if (sender_public_key->value->id != xmlSecOpenSSLKeyDataRsaId) {
         xmlFree(usig_alg); return false;
      }
      rsa = xmlSecOpenSSLKeyDataRsaGetRsa(sender_public_key->value);
        if (rsa == NULL) {
          xmlFree(usig_alg); return false;
        }
        key_size = RSA_size(rsa);
    } 
    else if (strcmp(usig_alg, (char*)xmlSecHrefDsaSha1) == 0) {
      if (sender_public_key->value->id != xmlSecOpenSSLKeyDataDsaId) {
        xmlFree(usig_alg); return false;
      }
      dsa = xmlSecOpenSSLKeyDataDsaGetDsa(sender_public_key->value);
      if (dsa == NULL) {
        xmlFree(usig_alg); return false;
      }
      key_size = DSA_size(dsa);
    } 
    else {
      xmlFree(usig_alg);
      return false;
    }

    /* insure there is only the signature in str_split[1] */
    f = str1.find("&");
    std::string sig_str = str1.substr(0, f-1);   


    char *b64_signature = NULL;
    xmlSecByte *signature = NULL;

    /* get signature (unescape + base64 decode) */
    signature = (unsigned char*)(xmlMalloc(key_size+1));
    b64_signature = (char*)xmlURIUnescapeString(sig_str.c_str(), 0, NULL);
    xmlSecBase64Decode((xmlChar*)b64_signature, signature, key_size+1);

    /* compute signature digest */
    xmlChar* md;
    md = (xmlChar*)(xmlMalloc(20));
    char* digest = (char*)SHA1((unsigned char*)(str0.c_str()), str0.size(), md);

    if (digest == NULL) {
      xmlFree(b64_signature);
      xmlFree(signature);
      xmlFree(usig_alg);
      return false;
    }

    int status = 0;

    if (rsa) {
      status = RSA_verify(NID_sha1, (unsigned char*)digest, 20, signature, key_size, rsa);
    } 
    else if (dsa) {
      status = DSA_verify(NID_sha1, (unsigned char*)digest, 20, signature, key_size, dsa);
    }

    if (status == 0) {
      std::cout<<"Signature of the query is not valid"<<std::endl;
    }

    xmlFree(b64_signature);
    xmlFree(signature);
    xmlFree(digest);
    xmlFree(usig_alg);
   
    return true;
  }

  std::string BuildDeflatedQuery(const Arc::XMLNode& node) {
    //deflated, b64'ed and url-escaped
    std::string encoding("utf-8");
    std::string query;
    node.GetXML(query, encoding);

    unsigned long len;
    z_stream stream;
    xmlChar *out;
    len = query.length();
    out = (xmlChar*)(malloc(len * 2));

    stream.next_in = (Bytef*)(query.c_str());
    stream.avail_in = len;
    stream.next_out = out;
    stream.avail_out = len * 2;

    stream.zalloc = NULL;
    stream.zfree = NULL;
    stream.opaque = NULL;

    int rc;
    /* -MAX_WBITS to disable zib headers */
    rc = deflateInit2(&stream, Z_DEFAULT_COMPRESSION, Z_DEFLATED, -MAX_WBITS, 5, 0);
    if (rc == Z_OK) {
      rc = deflate(&stream, Z_FINISH);
      if (rc != Z_STREAM_END) {
        deflateEnd(&stream);
        if (rc == Z_OK) {
          rc = Z_BUF_ERROR;
        }
      } 
      else {
        rc = deflateEnd(&stream);
      }
    }
    if (rc != Z_OK) {
      free(out);
      std::cerr<<"Failed to deflate the data"<<std::endl;
      return std::string();
    }

    xmlChar* b64_out;
    b64_out = xmlSecBase64Encode(out, stream.total_out, 0);
    free(out);
    out = xmlURIEscapeStr(b64_out, NULL);
    std::string res((char*)out);

    xmlFree(b64_out);
    xmlFree(out);

    return res;
  }

} //namespace Arc
