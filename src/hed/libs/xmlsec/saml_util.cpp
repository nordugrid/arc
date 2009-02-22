#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>
#include <fstream>
#include <sstream>

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
#include <xmlsec/openssl/crypto.h>

#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/sha.h>
#include <openssl/rand.h>
#ifdef CHARSET_EBCDIC
#include <openssl/ebcdic.h>
#endif 

#include "XmlSecUtils.h"
#include "saml_util.h"

namespace Arc {

  std::string SignQuery(std::string query, SignatureMethod sign_method, std::string& privkey_file) {
 
    std::string ret;

    BIO* key_bio = BIO_new_file(privkey_file.c_str(), "rb");
    if (key_bio == NULL) {
     std::cout<<"Failed to open private key file: "<<privkey_file<<std::endl;
     return ret;
    }

    /* Add SigAlg */
    char *t;
    std::string new_query; 
    switch (sign_method) {
      case RSA_SHA1:
        t = (char*)xmlURIEscapeStr(xmlSecHrefRsaSha1, NULL);
        new_query.append("SAMLRequest=").append(query).append("&SigAlg=").append(t);
	xmlFree(t);
	break;
      case DSA_SHA1:
	t = (char*)xmlURIEscapeStr(xmlSecHrefDsaSha1, NULL);
        new_query.append("SAMLRequest=").append(query).append("&SigAlg=").append(t);
	xmlFree(t);
	break;
    }

    /* Build buffer digest */
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

    /* Calculate signature value */
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

    /* Add signature */
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

  //bool VerifyQuery(const std::string query, const xmlSecKey *sender_public_key) {
  bool VerifyQuery(const std::string query, const std::string& sender_cert_str) {

    xmlSecKey* sender_public_key = NULL;
    sender_public_key =  get_key_from_certstr(sender_cert_str);
    if(sender_public_key == NULL) { 
      std::cerr<<"Failed to get public key from the certificate string"<<std::endl; 
      return false; 
    }

    /* split query, the signature MUST be the last param of the query,
     * there could be more params in the URL; but they wouldn't be
     * covered by the signature */
    size_t f;
    f = query.find("&Signature=");
    if(f == std::string::npos) { std::cerr<<"Failed to find signature in the query"<<std::endl; return false; }

    std::string str0 = query.substr(0,f);
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
      xmlFree(b64_signature);
      xmlFree(signature);
      xmlFree(digest);
      xmlFree(usig_alg);
      return false;
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

    //std::ostringstream oss (std::ostringstream::out);
    //node.SaveToStream(oss);
    //query = oss.str();
   
    //Arc::XMLNode node1(query);
    //std::string query1;
    //node1.GetXML(query1, encoding);  

    //std::cout<<"Query:  "<<query<<std::endl;

    std::string deflated_str = DeflateData(query);

    std::string b64_str = Base64Encode(deflated_str);

    std::string escaped_str = URIEscape(b64_str);

    return escaped_str;
  }

  std::string Base64Encode(const std::string& data) {
    unsigned long len;
    xmlChar *b64_out = NULL;
    len = data.length();
    b64_out = xmlSecBase64Encode((xmlChar*)(data.c_str()), data.length(), 0);
    std::string ret;
    if(b64_out != NULL) {
      ret.append((char*)b64_out);
      xmlFree(b64_out);
    }
    return ret;
  }

  std::string Base64Decode(const std::string& data) {
    unsigned long len;
    xmlChar *out = NULL;
    len = data.length();
    out = (xmlChar*)(xmlMalloc(len*4));
    len = xmlSecBase64Decode((xmlChar*)(data.c_str()), out, len*4);
    std::string ret;
    if(out != NULL) {
      ret.append((char*)out, len);
      xmlFree(out);
    }
    return ret;
  }

  std::string URIEscape(const std::string& data) {
    xmlChar* out = xmlURIEscapeStr((xmlChar*)(data.c_str()), NULL);
    std::string ret;
    ret.append((char*)out);
    xmlFree(out);
    return ret;
  }

  std::string URIUnEscape(const std::string& data) {
    xmlChar* out = (xmlChar*)xmlURIUnescapeString(data.c_str(), 0,  NULL);
    std::string ret;
    ret.append((char*)out);
    xmlFree(out);
    return ret;
  }

  std::string DeflateData(const std::string& data) {
    unsigned long len;
    char *out;
    len = data.length();
    out = (char*)(malloc(len * 2));
    z_stream stream;
    stream.next_in = (Bytef*)(data.c_str());
    stream.avail_in = len;
    stream.next_out = (Bytef*)out;
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
    std::string ret;
    ret.append(out,stream.total_out);

    free(out);
    return ret;
  }

  std::string InflateData(const std::string& data) {
    unsigned long len;
    char *out;
    len = data.length();
    out = (char*)(malloc(len * 10));
    z_stream stream;

    stream.zalloc = NULL;
    stream.zfree = NULL;
    stream.opaque = NULL;

    stream.avail_in = len;
    stream.next_in = (Bytef*)(data.c_str());
    stream.total_in = 0;
    stream.avail_out = len*10;
    stream.total_out = 0;
    stream.next_out = (Bytef*)out;

    int rc;
    rc = inflateInit2(&stream, -MAX_WBITS);
    if (rc != Z_OK) {
      std::cerr<<"inflateInit failed"<<std::endl;
      free(out);
      return std::string();
    }

    rc = inflate(&stream, Z_FINISH);
    if (rc != Z_STREAM_END) {
      std::cerr<<"Failed to inflate"<<std::endl;
      inflateEnd(&stream);
      free(out);
      return std::string();
    }
    out[stream.total_out] = 0;
    inflateEnd(&stream);

    std::string ret;
    ret.append(out);

    free(out);
    return ret;
  }

  static bool is_base64(const char *message) {
    const char *c;
    c = message;
    while (*c != 0 && (isalnum(*c) || *c == '+' || *c == '/' || *c == '\n' || *c == '\r')) c++;
    while (*c == '=' || *c == '\n' || *c == '\r') c++;
    if (*c == 0) return true;
    return false;
  }

  bool BuildNodefromMsg(const std::string msg, Arc::XMLNode& node) {
    bool b64 = false;
    char* str = (char*)(msg.c_str());
    if (is_base64(msg.c_str())) {   
      str = (char*)malloc(msg.length());
      int r = xmlSecBase64Decode((xmlChar*)(msg.c_str()), (xmlChar*)str, msg.length());
      if (r >= 0) b64 = true;
      else {
        free(str);
        str = (char*)(msg.c_str());
      }
    }

    if (strchr(str, '<')) {
      Arc::XMLNode nd(str);
      if(!nd) { std::cerr<<"Message format unknown"<<std::endl; free(str); return false; }
      if (b64) free(str);
      nd.New(node);
      return true;
    }

    //if (strchr(str, '&') || strchr(str, '='))
    if(b64) free(str);
    return false;
  }  

} //namespace Arc
