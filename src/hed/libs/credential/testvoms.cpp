#ifdef WIN32

int main(void) {
  return -1;
}

#else

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string>
#include <iostream>
#include <fstream>
#include <arc/Logger.h>
#include "Credential.h"
#include "VOMSUtil.h"


int main(void) {
  Arc::LogStream cdest(std::cerr);
  Arc::Logger::getRootLogger().addDestination(cdest);
  Arc::Logger::getRootLogger().setThreshold(Arc::DEBUG);


  std::string in_file_ac("/tmp/x509up_u1000");
  std::string ca_cert_dir("/etc/grid-security/certificates");
  std::string ca_cert_file("");
  Arc::Credential proxy2(in_file_ac, in_file_ac, ca_cert_dir, "");
  std::vector<std::string> attributes;
  std::vector<std::string> vomscert_trust_dn;
  //vomscert_trust_dn.push_back("/O=Grid/O=NorduGrid/CN=host/arthur.hep.lu.se");
  vomscert_trust_dn.push_back("^/O=Grid/O=NorduGrid");
  vomscert_trust_dn.push_back("NEXT CHAIN");
  vomscert_trust_dn.push_back("/O=Grid/O=NorduGrid/OU=fys.uio.no/CN=Weizhong Qiang");
  vomscert_trust_dn.push_back("/O=Grid/O=NorduGrid/CN=NorduGrid Certification Authority");

  Arc::parseVOMSAC(proxy2, ca_cert_dir, ca_cert_file, vomscert_trust_dn, attributes); 
  int i;
  std::cout<<"Obtained "<<attributes.size()<<" attributes"<<std::endl;
  for(i=0; i<attributes.size(); i++) {
    std::cout<<"Line "<<i<<" of the attributes returned: "<<attributes[i]<<std::endl;
  }
  return 0;
}

#endif
