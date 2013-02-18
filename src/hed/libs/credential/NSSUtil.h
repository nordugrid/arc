#ifndef __ARC_NSSUTIL_H__
#define __ARC_NSSUTIL_H__

#include <string>

#include <arc/DateTime.h>

/// Code for handling Network Security Services (NSS) credentials.
namespace AuthN {
  
  /// NSS certificate information.
  struct certInfo {
    std::string certname;
    std::string subject_dn;
    std::string issuer_dn;
    unsigned long serial;
    Arc::Time start;
    Arc::Time end;
  };

  /**
   * Initializes nss library
   * @param configdir   full path to the nss db
   */
  bool nssInit(const std::string& configdir);  

  bool nssExportCertificate(const std::string& certname, const std::string& certfile);

  bool nssOutputPKCS12(const std::string certname, char* outfile, char* slotpw, char* p12pw);

  bool nssGenerateCSR(const std::string& privkey_name, const std::string& dn, const char* slotpw, const std::string& outfile, std::string& privk_str, bool ascii = true);

  void nssListUserCertificatesInfo(std::list<certInfo>& certInfolist);

  bool nssCreateCert(const std::string& csrfile, const std::string& issuername, const char* passwd, const int duration, const std::string& vomsacseq, std::string& outfile, bool ascii = true);

  bool nssImportCertAndPrivateKey(char* slotpw, const std::string& keyfile, const std::string& keyname, const std::string& certfile, const std::string& certname, const char* trusts = NULL, bool ascii = true);

  bool nssImportCert(char* slotpw, const std::string& certfile, const std::string& name, const char* trusts = NULL, bool ascii = true);
}

#endif /*__ARC_NSSUTIL_H__*/
