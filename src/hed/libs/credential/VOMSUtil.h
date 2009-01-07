#ifndef __ARC_VOMSUTIL_H__
#define __ARC_VOMSUTIL_H__

#include <vector>
#include <string>

#include <arc/ArcRegex.h>
#include <arc/credential/Credential.h>
#include <arc/credential/VOMSAttribute.h>

namespace Arc {

  typedef std::vector<std::string> VOMSTrustChain;

  typedef std::string VOMSTrustRegex;

  /** Stores definitions for making decision if VOMS server is trusted */
  class VOMSTrustList {
    private:
      std::vector<VOMSTrustChain> chains_;
      std::vector<RegularExpression*> regexs_;
    public:
      VOMSTrustList(void) { };
      /** Creates chain lists and regexps from plain list. 
        List is made of chunks delimited by elements containing
        pattern "NEXT CHAIN". Each chunk with more than one element
        is converted into one instance of VOMSTrustChain. Chunks
        with single element are converted to VOMSTrustChain if
        element does not have special symbols. Otherwise it is 
        treated as regular expression. Those symbols are '^','$'
        and '*'. */
      VOMSTrustList(const std::vector<std::string>& encoded_list);
      /** Creates chain lists and regexps from those specified in arguments.
        See AddChain() and AddRegex() for more information. */
      VOMSTrustList(const std::vector<VOMSTrustChain>& chains,const std::vector<VOMSTrustRegex>& regexs);
      ~VOMSTrustList(void);
      /** Adds chain of trusted DNs to list.
        During verification each signature of AC is checked against
        all stored chains. DNs of chain of certificate used for signing
        AC are compared against DNs stored in these chains one by one.
        If needed DN of issuer of last certificate is checked too.
        Comparison succeeds if DNs in at least one stored chain are
        same as those in certificate chain. Comparison stops when all DNs
        in stored chain are compared. If there are more DNs in stored chain
        than in certificate chain then comparison fails. Empty stored 
        list matches any certificate chain.
        Taking into account that certificate chains are verified down
        to trusted CA anyway, having more than one DN in stored
        chain seems to be useless. But such feature may be found useful
        by some very strict sysadmins. 
        ??? IMO,DN list here is not only for authentication, it is also kind
        of ACL, which means the AC consumer only trusts those DNs which
        issues AC.
        */
      VOMSTrustChain& AddChain(const VOMSTrustChain& chain);
      /** Adds empty chain of trusted DNs to list. */
      VOMSTrustChain& AddChain(void);
      /** Adds regular expression to list.
        During verification each signature of AC is checked against
        all stored regular expressions. DN of signing certificate
        must match at least one of stored regular expressions. */
      RegularExpression& AddRegex(const VOMSTrustRegex& reg);
      int SizeChains(void) const { return chains_.size(); };
      int SizeRegexs(void) const { return chains_.size(); };
      const VOMSTrustChain& GetChain(int num) const { return chains_[num]; };
      const RegularExpression& GetRegex(int num) const { return *(regexs_[num]); };
  };

  void InitVOMSAttribute(void);

  /**
   * !!! TO DOCUMENT !!!
   * Each argument MUST be explained properly. What is fqan?
   * What is target? What attributes are supported?
   * In worst case there must be reference to documentation.
   */
  int createVOMSAC(X509 *issuer, STACK_OF(X509) *issuerstack, X509 *holder,
                   EVP_PKEY *pkey, BIGNUM *serialnum,
                   std::vector<std::string> &fqan,
                   std::vector<std::string> &targets,
                   std::vector<std::string>& attributes,
                   ArcCredential::AC **ac, std::string voname,
                   std::string uri, int lifetime);

  /**Create AC(Attribute Certificate) with voms specific format.
   * @param codedac  The coded AC as output of this method
   * @param issuer_cred  The issuer credential which is used to sign the AC
   * @param holder_cred   The holder credential, the holder certificate
   *       is the one which carries AC
   * @param fqan          
   * !!! TO DOCUMENT: content/meaning of arguments, missing arguments.
   */
  bool createVOMSAC(std::string& codedac, Arc::Credential& issuer_cred,
                    Arc::Credential& holder_cred,
                    std::vector<std::string> &fqan,
                    std::vector<std::string> &targets,
                    std::vector<std::string>& attributes, 
                    std::string &voname, std::string &uri, int lifetime);

  /*
   * !!! TO DOCUMENT !!!
   */
  bool addVOMSAC(ArcCredential::AC** &aclist, std::string &codedac);

  /**Parse the certificate, and output the attributes.
   * @param holder  The proxy certificate which includes the voms 
   *          specific formated AC.
   * @param ca_cert_dir  The trusted certificates which are used to 
   *          verify the certificate which is used to sign the AC
   * @param ca_cert_file The same as ca_cert_dir except it is a file
   *          instead of a directory. Only one of them need to be set
   * @param vomsdir  The directory which include *.lsc file for each vo.
   *          For instance, a vo called "knowarc.eu" should
   *          have file $prefix/vomsdir/knowarc/voms.knowarc.eu.lsc which
   *          contains on the first line the DN of the VOMS server, and on
   *          the second line the corresponding CA DN:
   *                   /O=Grid/O=NorduGrid/OU=KnowARC/CN=voms.knowarc.eu
   *                   /O=Grid/O=NorduGrid/CN=NorduGrid Certification Authority
   *  See more in : https://twiki.cern.ch/twiki/bin/view/LCG/VomsFAQforServiceManagers
   * @param output  The parsed attributes (Role and Generic Attribute) . 
   *           Each attribute is stored in element of a vector as a string.
   *           It is up to the consumer to understand the meaning of the
   *           attribute. An example is as following:
   *   TO DOCUMENT: some attributes generated by this function are
   *                as stored in AC, those must be described. For
   *                example there are no VO= and grantor= in VOMS AC.
   * 		      grantor=knowarc://squark.uio.no:15011/knowarc:Degree=PhD
   * 		      voname=knowarc://squark.uio.no:15011
   *     	      VO=knowarc
   * 		      VO=knowarc/Group=UiO
   * 		      VO=knowarc/Group=UiO/Role=technician
   */
  bool parseVOMSAC(X509* holder, const std::string& ca_cert_dir,
                   const std::string& ca_cert_file, 
                   const VOMSTrustList& vomscert_trust_dn,
                   std::vector<std::string>& output);

  /**Parse the certificate. The same as the above one */
  bool parseVOMSAC(Arc::Credential& holder_cred,
                   const std::string& ca_cert_dir,
                   const std::string& ca_cert_file, 
                   const VOMSTrustList& vomscert_trust_dn,
                   std::vector<std::string>& output);

}// namespace Arc

#endif /* __ARC_VOMSUTIL_H__ */

