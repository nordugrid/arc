#ifndef __ARC_VOMSUTIL_H__
#define __ARC_VOMSUTIL_H__

#include <vector>
#include <string>

#include <arc/ArcRegex.h>
#include <arc/credential/VOMSAttribute.h>
#include <arc/credential/Credential.h>

namespace Arc {

  /** \addtogroup credential
   *  @{ */

  typedef std::vector<std::string> VOMSTrustChain;

  typedef std::string VOMSTrustRegex;

  /// Represents VOMS attribute part of a credential.
  class VOMSACInfo {
   public:
    // Not all statuses are implemented
    typedef enum {
      Success = 0,
      CAUnknown = (1<<0), // Signed by VOMS certificate of unknow CA
      CertRevoked = (1<<1), // Signed by revoked VOMS certificate
      LSCFailed = (1<<2), // Failed while matching VOMS attr. against LSC files
      TrustFailed = (1<<2), // Failed matching VOMS attr. against specified trust list
      X509ParsingFailed = (1<<3), // Failed while parsing at X509 level
      ACParsingFailed = (1<<4), // Failed while parsing at AC level
      InternalParsingFailed = (1<<5), // Failed while parsing internal VOMS structures
      TimeValidFailed = (1<<6), // VOMS attributes are not valid yet or expired
      IsCritical = (1<<7), // VOMS extension was marked as critical (unusual but not error)
      ParsingError = (X509ParsingFailed | ACParsingFailed | InternalParsingFailed), // Mask to test if status represents any failure caused by failed parsing
      ValidationError = (CAUnknown | CertRevoked | LSCFailed | TrustFailed | TimeValidFailed), // Mask to test if status represents any failure caused by validation rules
      Error = (0xffff & ~IsCritical) // Mask to test if status represents any failure
    } status_t;
    std::string voname;
    std::string holder;
    std::string issuer;
    std::string target;
    std::vector<std::string> attributes;
    Time from;
    Time till;
    //Period validity;
    unsigned int status;
    VOMSACInfo(void):from(-1),till(-1),status(0) { };
  };

  /// Stores definitions for making decision if VOMS server is trusted.
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
      void AddElement(const std::vector<std::string>& encoded_list);
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
  /*
  int createVOMSAC(X509 *issuer, STACK_OF(X509) *issuerstack, X509 *holder,
                   EVP_PKEY *pkey, BIGNUM *serialnum,
                   std::vector<std::string> &fqan,
                   std::vector<std::string> &targets,
                   std::vector<std::string>& attributes,
                   ArcCredential::AC **ac, std::string voname,
                   std::string uri, int lifetime);
  */

  /**Create AC(Attribute Certificate) with voms specific format.
   * @param codedac The coded AC as output of this method
   * @param issuer_cred   The issuer credential which is used to sign the AC
   * @param holder_cred   The holder credential, the holder certificate
   *			   is the one which carries AC
   * @param fqan    The AC_IETFATTR. According to the definition of voms, the fqan
   *                      will be like /Role=Employee/Group=Tester/Capability=NULL
   * @param targets The list of targets which are supposed to consume this AC
   * @param attributes  The AC_FULL_ATTRIBUTES.  Accoding to the definition of voms,
   *                      the attributes will be like "qualifier::name=value"
   * @param voname  The vo name
   * @param uri   The uri of this vo, together with voname, it will be
   *          as the granter of this AC
   * @param lifetime  The lifetime of this AC   *
   */
  bool createVOMSAC(std::string& codedac, Credential& issuer_cred,
                    Credential& holder_cred,
                    std::vector<std::string> &fqan,
                    std::vector<std::string> &targets,
                    std::vector<std::string>& attributes, 
                    std::string &voname, std::string &uri, int lifetime);

  /**Add decoded AC string into a list of AC objects
   * @param aclist 	The list of AC objects (output)
   * @param acorder       The order of AC objects (output)
   * @param decodedac	The AC string that is decoded from the string
   * 			returned from voms server (input)
   */
  bool addVOMSAC(ArcCredential::AC** &aclist, std::string &acorder, std::string &decodedac);

  /**Parse the certificate, and output the attributes.
   * @param holder  The proxy certificate which includes the voms 
   *          specific formatted AC.
   * @param ca_cert_dir  The trusted certificates which are used to 
   *          verify the certificate which is used to sign the AC
   * @param ca_cert_file The same as ca_cert_dir except it is a file
   *          instead of a directory. Only one of them need to be set
   * @param vomsdir  The directory which include *.lsc file for each vo.
   *          For instance, a vo called "knowarc.eu" should
   *          have file vomsdir/knowarc/voms.knowarc.eu.lsc which
   *          contains on the first line the DN of the VOMS server, and on
   *          the second line the corresponding CA DN:
   *                   /O=Grid/O=NorduGrid/OU=KnowARC/CN=voms.knowarc.eu
   *                   /O=Grid/O=NorduGrid/CN=NorduGrid Certification Authority
   *  See more in : https://twiki.cern.ch/twiki/bin/view/LCG/VomsFAQforServiceManagers
   * @param vomscert_trust_dn List of VOMS trust chains
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
   * 		for AC_FULL_ATTRIBUTES, the voname + hostname is added:
   * 		      /voname=knowarc.eu/hostname=arthur.hep.lu.se:15001//knowarc.eu/coredev:attribute1=1
   * 		for AC_IETFATTR, the 'VO' (voname) is added:
   * 		      /VO=knowarc.eu/Group=coredev/Role=NULL/Capability=NULL
   * 		      /VO=knowarc.eu/Group=testers/Role=NULL/Capability=NULL
   * 		some other redundant attributes is provided:
   * 		      voname=knowarc.eu/hostname=arthur.hep.lu.se:15001
   * @param verify  true: Verify the voms certificate is trusted based on the 
   * 		    ca_cert_dir/ca_cert_file which specifies the CA certificates, 
   * 		    and the vomscert_trust_dn which specifies the trusted DN chain
   * 		    from voms server certificate to CA certificate.
   * 		    false: Not verify, which means the issuer of AC (voms server
   * 		    certificate is supposed to be trusted by default).
   * 		    In this case the parameters 'ca_cert_dir', 
   * 		    'ca_cert_file' and 'vomscert_trust_dn' will not effect, and 
   * 		    may be left empty.
   * 		    This case is specifically used by 'arcproxy --info' to list 
   * 		    all of the attributes in AC, and not to need to verify if
   * 		    the AC's issuer is trusted.
   *
   * @param reportall If set to true fills output with all attributes
   *                including those which failed passing test procedures.
   *                Validity of attributes can be checked through status 
   *                members of output items.
   *                Combination of verify=true and reportall=true provides 
   *                most information.
   *
   */
  bool parseVOMSAC(X509* holder,
                   const std::string& ca_cert_dir,
                   const std::string& ca_cert_file, 
                   const std::string& vomsdir, 
                   VOMSTrustList& vomscert_trust_dn,
                   std::vector<VOMSACInfo>& output, 
                   bool verify = true, bool reportall = false);

  /**Parse the certificate. Similar to above one, but collects information
    From all certificates in a chain. */
  bool parseVOMSAC(const Credential& holder_cred,
                   const std::string& ca_cert_dir,
                   const std::string& ca_cert_file, 
                   const std::string& vomsdir, 
                   VOMSTrustList& vomscert_trust_dn,
                   std::vector<VOMSACInfo>& output,
                   bool verify = true, bool reportall = false);

  /**Parse the certificate or a chain of certificates, in string format */
  bool parseVOMSAC(const std::string& cert_str,
                   const std::string& ca_cert_dir,
                   const std::string& ca_cert_file,
                   const std::string& vomsdir,
                   VOMSTrustList& vomscert_trust_dn,
                   std::vector<VOMSACInfo>& output,
                   bool verify = true, bool reportall = false); 
 
  /**Decode the data which is encoded by voms server. Since voms code uses some specific
  * coding method (not base64 encoding), we simply copy the method from voms code to here*/
  char *VOMSDecode(const char *data, int size, int *j);

  /**Encode the data with base64 encoding */
  char *VOMSEncode(const char *data, int size, int *j);

  /**Extract the needed field from the certificate.
   * @param u  The proxy certificate which includes the voms
   *          specific formatted AC.
   * @param property The property that caller would get, 
   *          including: dn, voms:vo, voms:role, voms:group 
   * @param ca_cert_dir 
   * @param ca_cert_file
   * @param vomsdir
   * @param voms_trust_list  the dn chain that is trusted when parsing voms AC
   * \since Changed in 4.1.0. Provide ability to query credential for VOMS
   *  nickname attribute.
  */
  std::string getCredentialProperty(const Arc::Credential& u, const std::string& property,
                                    const std::string& ca_cert_dir = std::string(""),
                                    const std::string& ca_cert_file = std::string(""),
                                    const std::string& vomsdir = std::string(""),
                                    const std::vector<std::string>& voms_trust_list = std::vector<std::string>());

  std::string VOMSFQANToFull(const std::string& vo, const std::string& fqan);

  std::string VOMSFQANFromFull(const std::string& attribute);

  /**Encode the VOMS AC list into ASN1, so that the result can be used 
   * to insert into X509 as extension.
   * @param ac_seq  The input string includes a list of AC 
                    with VOMS_AC_HEADER and VOMS_AC_TRAILER as separator
   * @param asn1    The encoded value as output
  */
  bool VOMSACSeqEncode(const std::string& ac_seq, std::string& asn1);

  /**Encode the VOMS AC list into ASN1, so that the result can be used
   * to insert into X509 as extension.
   * @param acs     The input list includes a list of AC
   * @param asn1    The encoded value as output
  */
  bool VOMSACSeqEncode(const std::list<std::string> acs, std::string& asn1);

  /** @} */

}// namespace Arc

#endif /* __ARC_VOMSUTIL_H__ */

