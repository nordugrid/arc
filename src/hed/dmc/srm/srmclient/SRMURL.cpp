#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/StringConv.h>

#include "SRMURL.h"

namespace ArcDMCSRM {
  
  SRMURL::SRMURL(std::string url): URL(url) {
    portdefined = false;
    if(protocol != "srm") { 
      valid=false;
      return; 
    };
    valid=true;
    if(port <= 0) port=8443;
    else portdefined = true;
    srm_version = SRM_URL_VERSION_2_2; // v2.2 unless explicitly made otherwise
    if(HTTPOption("SFN", "") != "") {
      filename=HTTPOption("SFN");
      isshort=false;
      path = '/'+path;
      for(;path.size() > 1;) {
        if(path[1] != '/') break;
        path.erase(0,1);
      };
      if (path[path.size()-1] == '1') srm_version = SRM_URL_VERSION_1;
    }
    else {
      if(path.length() > 0) filename=path.c_str()+1; // Skip leading '/'
      path = "/srm/managerv2";
      isshort=true;
    }
  }  
  
  void SRMURL::SetSRMVersion(const std::string& version) {
    if (version.empty()) return;
    if (version == "1") {
      srm_version = SRM_URL_VERSION_1;
      path = "/srm/managerv1";
    }
    else if (version == "2.2") {
      srm_version = SRM_URL_VERSION_2_2;
      path = "/srm/managerv2";
    }
    else {
      srm_version = SRM_URL_VERSION_UNKNOWN; 
    }
  }
  
  std::string SRMURL::ContactURL(void) const {
    if(!valid) return "";
    std::string contact_protocol("httpg");
    if((Option("protocol") == "tls") || (Option("protocol") == "ssl")) {
      contact_protocol = "https";
    }
    if(!Option("protocol").empty()) {
      return (contact_protocol+"://"+host+":"+Arc::tostring(port)+";protocol="+Option("protocol")+path);
    }
    return (contact_protocol+"://"+host+":"+Arc::tostring(port)+path);
  }
  
  std::string SRMURL::BaseURL(void) const {
    if(!valid) return "";
    return (protocol+"://"+host+":"+Arc::tostring(port)+path+"?SFN=");
  }
  
  std::string SRMURL::FullURL(void) const {
    if(!valid) return "";
    return (protocol+"://"+host+":"+Arc::tostring(port)+path+"?SFN="+filename);
  }
  
  std::string SRMURL::ShortURL(void) const {
    return (protocol+"://"+host+":"+Arc::tostring(port)+"/"+filename);
  }
  
} // namespace ArcDMCSRM
