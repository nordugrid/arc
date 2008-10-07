#ifndef __ARC_VOMSUTIL_H__
#define __ARC_VOMSUTIL_H__

#include <vector>
#include <string>

#include "Credential.h"

namespace ArcLib {

  void InitVOMSAttribute(void);

  /**
   *
   *
   */
  int createVOMSAC(X509 *issuer, STACK_OF(X509) *issuerstack, X509 *holder, EVP_PKEY *pkey, BIGNUM *serialnum,
             std::vector<std::string> &fqan, std::vector<std::string> &targets, std::vector<std::string>& attributes,
             AC **ac, std::string voname, std::string uri, int lifetime);

  /**Create AC(Attribute Certificate) with voms specific format.
   * @param codedac  The coded AC as output of this method
   * @param issuer_cred  The issuer credential which is used to sign the AC
   * @param holer_cred   The holder credential, the holder certificate is the one which carries AC
   * @param fqan          
   */
  bool createVOMSAC(std::string& codedac, Credential& issuer_cred, Credential& holder_cred,
             std::vector<std::string> &fqan, std::vector<std::string> &targets, std::vector<std::string>& attributes, 
             std::string &voname, std::string &uri, int lifetime);

  bool addVOMSAC(AC** &aclist, std::string &codedac);

  /**Parse the certificate, and output the attributes.
   * @param holder  The proxy certificate which includes the voms specific formated AC.
   * @param ca_cert_dir  The trusted certificates which are used to verify the certificate which is used
   *                     to sign the AC
   * @param ca_cert_file The same as ca_cert_dir except it is a file instead of a directory. Only one of them 
   *                     need to be set
   * @param vomsdir  The directory which include *.lsc file for each vo. For instance, a vo called "knowarc" should
   *                 has file $prefix/vomsdir/knowarc/voms.knowarc.eu.lsc which contains on the first line the DN 
   *                 of the VOMS server, and on the second line the corresponding CA DN:
   *                   /O=Grid/O=NorduGrid/OU=KnowARC/CN=voms.knowarc.eu
   *                   /O=Grid/O=NorduGrid/CN=NorduGrid Certification Authority
   *                 See more in : https://twiki.cern.ch/twiki/bin/view/LCG/VomsFAQforServiceManagers
   * @param output  The parsed attributes (Role and Generic Attribute) . 
   *                Each of the attributes in stored in a string as an element of a vector.
   *                It is up to the consumer to understand the meaning of the attribute.
   *                An example is as following:
   * 		      grantor=knowarc://squark.uio.no:15011/knowarc:Degree=PhD
   * 		      voname=knowarc://squark.uio.no:15011
   *     	      VO=knowarc
   * 		      VO=knowarc/Group=UiO
   * 		      VO=knowarc/Group=UiO/Role=technician
   */
  bool parseVOMSAC(X509* holder, const std::string& ca_cert_dir, const std::string& ca_cert_file, 
             const std::vector<std::string>& vomscert_trust_dn, std::vector<std::string>& output);

  /**Parse the certificate. The same as the above one */
  bool parseVOMSAC(Credential& holder_cred, const std::string& ca_cert_dir, const std::string& ca_cert_file, 
             const std::vector<std::string>& vomscert_trust_dn, std::vector<std::string>& output);

}// namespace ArcLib

#endif /* __ARC_VOMSUTIL_H__ */

