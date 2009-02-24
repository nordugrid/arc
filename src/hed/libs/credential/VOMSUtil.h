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
        and '*'. 
        Trusted chains can be congicured in two ways: 
        one way is:
            <tls:VOMSCertTrustDNChain>
              <tls:VOMSCertTrustDN>/O=Grid/O=NorduGrid/CN=host/arthur.hep.lu.se</tls:VOMSCertTrustDN>
              <tls:VOMSCertTrustDN>/O=Grid/O=NorduGrid/CN=NorduGrid Certification Authority</tls:VOMSCertTrustDN>
              <tls:VOMSCertTrustDN>----NEXT CHAIN---</tls:VOMSCertTrustDN>
              <tls:VOMSCertTrustDN>/DC=ch/DC=cern/OU=computers/CN=voms.cern.ch</tls:VOMSCertTrustDN>
              <tls:VOMSCertTrustDN>/DC=ch/DC=cern/CN=CERN Trusted Certification Authority</tls:VOMSCertTrustDN>
            </tls:VOMSCertTrustDNChain>
        the other way is:
            <tls:VOMSCertTrustDNChain>
              <tls:VOMSCertTrustDN>/O=Grid/O=NorduGrid/CN=host/arthur.hep.lu.se</tls:VOMSCertTrustDN>
              <tls:VOMSCertTrustDN>/O=Grid/O=NorduGrid/CN=NorduGrid Certification Authority</tls:VOMSCertTrustDN>
            </tls:VOMSCertTrustDNChain>
            <tls:VOMSCertTrustDNChain>
              <tls:VOMSCertTrustDN>/DC=ch/DC=cern/OU=computers/CN=voms.cern.ch</tls:VOMSCertTrustDN>
              <tls:VOMSCertTrustDN>/DC=ch/DC=cern/CN=CERN Trusted Certification Authority</tls:VOMSCertTrustDN>
            </tls:VOMSCertTrustDNChain>
        each chunk is supposed to contain a suit of DN of trusted 
        certificate chain, in which the first DN is the DN of the 
        certificate (cert0) which is used to sign the Attribute Certificate
        (AC), the second DN is the DN of the issuer certificate(cert1) 
        which is used to sign cert0. So if there are one or more intermediate 
        issuers, then there should be 3 or more than 3 DNs in this 
        chunk (considering cert0 and the root certificate, plus the intermediate
        certificate) .
      */
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
      int SizeRegexs(void) const { return regexs_.size(); };
      const VOMSTrustChain& GetChain(int num) const { return chains_[num]; };
      const RegularExpression& GetRegex(int num) const { return *(regexs_[num]); };
  };

  void InitVOMSAttribute(void);

  /* This method is used to create an AC. It is supposed 
   * to be used by the voms server
   * @param issuer 	The issuer which will be used to sign the AC, it is also 
   * 			the voms server certificate
   * @param issuerstack	The stack of the issuer certificates that issue the 
   * 			voms server certificate. If the voms server certificate
   * 			is issued by a root CA (self-signed), then this param 
   * 			is empty.
   * @param holder	The certificate of the holder of this AC. It should be 
   * 			parsed from the peer that launches a AC query request
   * @param pkey	The key of the holder
   * @param fqan   	The AC_IETFATTR. According to the definition of voms, the fqan
   *                      will be like /Role=Employee/Group=Tester/Capability=NULL
   * @param attributes 	The AC_FULL_ATTRIBUTES.  Accoding to the definition of voms, 
   *                      the attributes will be like "qualifier::name=value" 
   * @param target 	The list of targets which are supposed to consume this AC
   * @param ac 	   	The generated AC
   * @param voname	The vo name
   * @param uri		The uri of this vo, together with voname, it will be 
   *  			  as the grantor of this AC
   * @param lifetime	The lifetime of this AC
   */
  int createVOMSAC(X509 *issuer, STACK_OF(X509) *issuerstack, X509 *holder,
                   EVP_PKEY *pkey, BIGNUM *serialnum,
                   std::vector<std::string> &fqan,
                   std::vector<std::string> &targets,
                   std::vector<std::string>& attributes,
                   ArcCredential::AC **ac, std::string voname,
                   std::string uri, int lifetime);

  /**Create AC(Attribute Certificate) with voms specific format.
   * @param codedac	  The coded AC as output of this method
   * @param issuer_cred   The issuer credential which is used to sign the AC
   * @param holder_cred   The holder credential, the holder certificate
   *			   is the one which carries AC
   * The rest arguments are the same as the above method          
   */
  bool createVOMSAC(std::string& codedac, Arc::Credential& issuer_cred,
                    Arc::Credential& holder_cred,
                    std::vector<std::string> &fqan,
                    std::vector<std::string> &targets,
                    std::vector<std::string>& attributes, 
                    std::string &voname, std::string &uri, int lifetime);

  /**Add decoded AC string into a list of AC objects
   * @param aclist 	The list of AC objects
   * @param decodedac	The AC string that is decoded from the string
   * 			returned from voms server
   */
  bool addVOMSAC(ArcCredential::AC** &aclist, std::string &decodedac);

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
   *           attribute.
   *		There are two types of attributes stored in VOMS AC: 
   *		AC_IETFATTR, AC_FULL_ATTRIBUTES.
   *		The AC_IETFATTR will be like /Role=Employee/Group=Tester/Capability=NULL
   *		The AC_FULL_ATTRIBUTES will be like knowarc:Degree=PhD (qualifier::name=value)
   *            In order to make the output attribute values be identical, the voms server
   *            information is added as prefix of the original attributes in AC.
   * 		for AC_FULL_ATTRIBUTES, the 'grantor' (voname + uri) is added:
   * 		      grantor=knowarc://squark.uio.no:15011/knowarc:Degree=PhD
   * 		for AC_IETFATTR, the 'VO' (voname) is added:
   *     	      VO=knowarc
   * 		      VO=knowarc/Group=UiO
   * 		      VO=knowarc/Group=UiO/Role=technician
   *
   * 		some other redundant attributes is provided:
   * 		      voname=knowarc://squark.uio.no:15011
   * @param verify  true: Verify the voms certificate is trusted based on the 
   * 		    ca_cert_dir/ca_cert_file which specifies the CA certificates, 
   * 		    and the vomscert_trust_dn which specifies the trusted DN chain
   * 		    from voms server certificate to CA certificate.
   *
   * 		    false: Not verify, which means the issuer of AC (voms server
   * 		    certificate is supposed to be trusted by default).
   * 		    In this case the parameters 'ca_cert_dir', 
   * 		    'ca_cert_file' and 'vomscert_trust_dn' will not effect, and 
   * 		    should be set as empty.
   * 		    This case is specifically used by 'arcproxy --info' to list 
   * 		    all of the attributes in AC, and not to need to verify if
   * 		    the AC's issuer is trusted.
   */
  bool parseVOMSAC(X509* holder, const std::string& ca_cert_dir,
                   const std::string& ca_cert_file, 
                   const VOMSTrustList& vomscert_trust_dn,
                   std::vector<std::string>& output, bool verify = true);

  /**Parse the certificate. The same as the above one */
  bool parseVOMSAC(Arc::Credential& holder_cred,
                   const std::string& ca_cert_dir,
                   const std::string& ca_cert_file, 
                   const VOMSTrustList& vomscert_trust_dn,
                   std::vector<std::string>& output, bool verify = true);
  
  /**Decode the data which is encoded by voms server. Since voms code uses some specific
  * coding method (not base64 encoding), we simply copy the method from voms code to here*/
  char *VOMSDecode(const char *data, int size, int *j);

}// namespace Arc

#endif /* __ARC_VOMSUTIL_H__ */

