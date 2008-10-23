#ifndef __ARC_WSSUTIL_H__
#define __ARC_WSSUTIL_H__

#include <vector>
#include <string>

#include <xmlsec/crypto.h>

#include <arc/XMLNode.h>

///Some utility methods for using xml security library (http://www.aleksey.com/xmlsec/)
namespace Arc {
  /**callback method for inputing passphrase of key file*/
  int passphrase_callback(char* buf, int size, int rwflag, void *);
  /**Initialize the xml security library, it should be called before the xml security 
   * functionality is used.*/
  bool init_xmlsec(void);
  /**Finalize the xml security library*/
  bool final_xmlsec(void);
  /**Get certificate in string format from certificate file*/
  std::string get_cert_str(const char* certfile);
  /**Get key in xmlSecKey structure from key in string format*/
  xmlSecKey* get_key_from_keystr(const std::string& value);
  /**Get key in xmlSecKey structure from key file*/
  xmlSecKey* get_key_from_keyfile(const char* keyfile);
  /**Get public key in string format from certificate file*/
  std::string get_key_from_certfile(const char* certfile);
  /**Get public key in xmlSecKey structure from certificate string (the string under 
   * "-----BEGIN CERTIFICATE-----" and "-----END CERTIFICATE-----")
   */
  xmlSecKey* get_key_from_certstr(const std::string& value);
  /**Load private or public key from a key file into key manager*/
  xmlSecKeysMngrPtr load_key_from_keyfile(xmlSecKeysMngrPtr* keys_manager, const char* keyfile);
  /**Load public key from a certificate file into key manager*/
  xmlSecKeysMngrPtr load_key_from_certfile(xmlSecKeysMngrPtr* keys_manager, const char* certfile);
  /**Load public key from a certificate string into key manager*/
  xmlSecKeysMngrPtr load_key_from_certstr(xmlSecKeysMngrPtr* keys_manager, const std::string& certstr);
  /**Load trusted certificate from certificate file into key manager*/
  xmlSecKeysMngrPtr load_trusted_cert_file(xmlSecKeysMngrPtr* keys_manager, const char* cert_file);
  /**Load trusted certificate from cetrtificate string into key manager*/
  xmlSecKeysMngrPtr load_trusted_cert_str(xmlSecKeysMngrPtr* keys_manager, const std::string& cert_str);
  /**Load trusted cetificates from a file or directory into key manager*/
  xmlSecKeysMngrPtr load_trusted_certs(xmlSecKeysMngrPtr* keys_manager, const char* cafile, const char* capath);
  /**Generate a new child XMLNode with specified name*/
  XMLNode get_node(XMLNode& parent,const char* name);

}// namespace Arc

#endif /* __ARC_WSSSUTIL_H__ */

