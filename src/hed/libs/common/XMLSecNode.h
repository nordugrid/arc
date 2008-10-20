#ifndef __ARC_XMLSEC_H__
#define __ARC_XMLSEC_H__

#include <string>

#include <arc/XMLNode.h>

namespace Arc {
/// Extends XMLNode class to support XML security operation.
/** All XMLNode methods are exposed by inheriting from XMLNode. XMLSecNode itself 
does not own node, instead it uses the node from the base class XMLNode.*/
class XMLSecNode: public XMLNode {
  public:
    typedef enum {
      RSA_SHA1,
      DSA_SHA1
    } SignatureMethod;
    typedef enum {
      TRIPLEDES,
      AES_128,
      AES_256,
      DEFAULT
    } SymEncryptionType;
   /** Create a object based on an XMLNode instance.*/ 
   XMLSecNode(XMLNode& node);
   ~XMLSecNode(void);
   /**Add the signature template for later signing.
    *@param id_name The identifier name under this node which will be used for the 
                     <Signature/> to refer to.
    *@param sign_method  The sign method for signing. Two options now, RSA_SHA1, DSA_SHA1
    */
   void AddSignatureTemplate(const std::string& id_name, const SignatureMethod sign_method, 
      const std::string& incl_namespaces = "");
   /** Sign this node (identified by id_name).
    *@param privkey_file  The private key file. The private key is used for signing
    *@param cert_file     The certificate file. The certificate is used as the <KeyInfo/> part
                          of the <Signature/>; <KeyInfo/> will be used for the other end to verify
                          this <Signature/> 
    *@param incl_namespaces  InclusiveNamespaces for Tranform in Signature
    */
   bool SignNode(const std::string& privkey_file, const std::string& cert_file);
   /** Verify the signature under this node
    *@param id_name   The id of this node, which is used for identifying the node
    *@param ca_file   The CA file which used as trused certificate when verify the certificate in the 
                      <KeyInfo/> part of <Signature/> 
    *@param ca_path   The CA directory; either ca_file or ca_path should be set.
    */
   bool VerifyNode(const std::string& id_name, const std::string& ca_file, const std::string& ca_path, 
      bool verify_trusted = true);
   /** Encrypt this node, after encryption, this node will be replaced by the encrypted node
    *@param cert_file   The certificate file, the public key parsed from this certificate is used to 
                        encrypted the symmetric key, and then the symmetric key is used to encrypted the node
    *@param encrpt_type The encryption type when encrypting the node, four option in SymEncryptionType
    *@param verify_trusted Verify trusted certificates or not. If set to false, then only the signature will be 
    *                      checked (by using the public key from KeyInfo).
    */
   bool EncryptNode(const std::string& cert_file, const SymEncryptionType encrpt_type);
   /** Decrypt the <xenc:EncryptedData/> under this node, the decrypted node will be output in the second
    *argument of DecryptNode method. And the <xenc:EncryptedData/> under this node will be removed after
    *decryption.
    *@param privkey_file   The private key file, which is used for decrypting
    *@param decrypted_node Output the decrypted node
    */
   bool DecryptNode(const std::string& privkey_file, Arc::XMLNode& decrypted_node);
};

} // namespace Arc 

#endif /* __ARC_XMLSEC_H__ */

