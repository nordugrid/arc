#include <arc/StringConv.h>

#include "SRMURL.h"

//namespace Arc {
  
  std::string SRMURL::empty("");
  
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
  
  SRMURL::SRMURL(const char* url) try: URL(url) {
    if(protocol != "srm") { valid=false; return; };
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
      return;
    };
    // Add v1 specific part
    if(path.length() > 0) filename=path.c_str()+1; // Skip leading '/'
    //path="srm/srm.1.1.endpoint";
    path="/srm/managerv1";
    isshort=true;
  } catch (std::exception e) {
    valid=false;
  }
  
  bool SRMURL::GSSAPI(void) const {
    try {
      const std::string proto_val =
            ((std::map<std::string,std::string>&)(Options()))["protocol"];
      if(proto_val == "gssapi") return true;
    } catch (std::exception e) { };
    return false;
  }
  
  SRM22URL::SRM22URL(const char* url) try: SRMURL(url) {
    if(protocol != "srm") { valid=false; return; };
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
      return;
    };
    if(path.length() > 0) filename=path.c_str()+1; // Skip leading '/'
    path="/srm/managerv2";
    isshort=true;
  } catch (std::exception e) {
    valid=false;
  }

//} // namespace Arc
