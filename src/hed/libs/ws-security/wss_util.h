#ifndef __ARC_WSSUTIL_H__
#define __ARC_WSSUTIL_H__

#include <vector>
#include <string>

#include <xmlsec/crypto.h>

#include <arc/XMLNode.h>

namespace Arc {
  int passphrase_callback(char* buf, int size, int rwflag, void *);
  bool init_xmlsec(void);
  bool final_xmlsec(void);
  std::string get_cert_str(const char* certfile);
  xmlSecKey* get_key_from_keystr(const std::string& value);
  std::string get_key_from_certfile(const char* certfile);
  xmlSecKey* get_key_from_certstr(const std::string& value);
  xmlSecKeysMngrPtr load_key_from_keyfile(xmlSecKeysMngrPtr* keys_manager, const char* keyfile);
  xmlSecKeysMngrPtr load_key_from_certfile(xmlSecKeysMngrPtr* keys_manager, const char* certfile);
  xmlSecKeysMngrPtr load_trusted_cert(xmlSecKeysMngrPtr* keys_manager, const std::string& cert_str);
  xmlSecKeysMngrPtr load_trusted_certs(xmlSecKeysMngrPtr* keys_manager, const char* cafile, const char* capath);
  XMLNode get_node(XMLNode& parent,const char* name);

}// namespace Arc

#endif /* __ARC_WSSSUTIL_H__ */

