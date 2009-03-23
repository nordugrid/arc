#include <arc/StringConv.h>

#include "SRMURL.h"

//namespace Arc {
  
  SRMURL::SRMURL(std::string url) try: URL(url) {
    if(protocol != "srm") { 
      valid=false;
      return; 
    };
    valid=true;
    if(port <= 0) port=8443;
    std::string::size_type p = path.find("?SFN=");
    if(p != std::string::npos) {
      filename=path.c_str()+p+5;
      path.resize(p);
      isshort=false;
      for(;path.size() > 1;) {
        if(path[1] != '/') break;
        path.erase(0,1);
      };
      if (path[path.size()-1] == '1') srm_version = SRM_URL_VERSION_1;
      else if (path[path.size()-1] == '2') srm_version = SRM_URL_VERSION_2_2;
    }
    else {
      if(path.length() > 0) filename=path.c_str()+1; // Skip leading '/'
      path = "";
      isshort=true;
      srm_version = SRM_URL_VERSION_UNKNOWN;
    }
  } catch (std::exception e) {
    valid=false;
  }  
  
  std::string SRMURL::empty("");
  
  
  void SRMURL::SetSRMVersion(std::string version) {
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
    if(!valid) return empty;
    return ("httpg://"+host+":"+Arc::tostring(port)+path);
  }
  
  std::string SRMURL::BaseURL(void) const {
    if(!valid) return empty;
    return (protocol+"://"+host+":"+Arc::tostring(port)+path+"?SFN=");
  }
  
  std::string SRMURL::FullURL(void) const {
    if(!valid) return empty;
    return (protocol+"://"+host+":"+Arc::tostring(port)+path+"?SFN="+filename);
  }
  
  std::string SRMURL::ShortURL(void) const {
    return (protocol+"://"+host+":"+Arc::tostring(port)+"/"+filename);
  }
  
  bool SRMURL::GSSAPI(void) const {
    try {
      const std::string proto_val =
            ((std::map<std::string,std::string>&)(Options()))["protocol"];
      if(proto_val == "gssapi") return true;
    } catch (std::exception e) { };
    return false;
  }

//} // namespace Arc
