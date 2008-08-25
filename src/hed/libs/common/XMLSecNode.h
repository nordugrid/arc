#ifndef __ARC_XMLSEC_H__
#define __ARC_XMLSEC_H__

#include <string>

#include <arc/XMLNode.h>

namespace Arc {
/// Extends XMLNode class to support XML security operation.
/** All XMLNode methods are exposed by inheriting from XMLNode. XMLSecNode itself 
does not own node, instead it uses the node from XMLNode.*/
class XMLSecNode: public XMLNode {
  public:
    typedef enum {
      RSA_SHA1,
      DSA_SHA1
    } SignatureMethod;
    typedef_enum {
      3DES,
      AES_128,
      AES_256,
      DEFAULT
    } SymEncryptionType;
   /** Create a object based on an XMLNode instance. */ 
   XMLSecNode(XMLNode& node);
   ~XMLSecNode(void);
   void AddSignatureTemplate(std::string& id_name, SignatureMethod sign_method);
   bool SignNode(std::string& privkey_file, std::string& cert_file);
   bool VerifyNode(std::string& id_name, std::string& ca_file, std::string& ca_path);
   bool EncryptNode(std::string& cert_file, SymEncryptionType encrpt_type);
   bool DecryptNode(std::string& privkey_file);
};

} // namespace Arc 

#endif /* __ARC_XMLSEC_H__ */

