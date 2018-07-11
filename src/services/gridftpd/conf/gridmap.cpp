#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string>
#include <fstream>
#include <arc/StringConv.h>
#include <arc/ArcConfigIni.h>
#include "environment.h"
#include "gridmap.h"


namespace gridftpd {

  /*
  bool file_user_list(const std::string& path,std::string &ulist) {
    std::ifstream f(path.c_str());
    if(! f.is_open() ) return false;
    for(;f.good();) {
      std::string rest;
      std::getline(f,rest);
      Arc::trim(rest," \t\r\n");
      std::string name = "";
      for(;rest.length() != 0;) {
        name=Arc::ConfigIni::NextArg(rest,' ','"');
      };
      if(name.length() == 0) continue;
      std::string::size_type pos;
      if((pos=ulist.find(name)) != std::string::npos) {
        if(pos!=0)
          if(ulist[pos-1] != ' ') { ulist+=" "+name; continue; };
        pos+=name.length();
        if(pos < ulist.length())
          if(ulist[pos] != ' ') { ulist+=" "+name; continue; };
      }
      else { ulist+=" "+name; };
    };
    f.close();
    return true;
  }

  bool file_user_list(const std::string& path,std::list<std::string> &ulist) {
    std::ifstream f(path.c_str());
    if(! f.is_open() ) return false;
    for(;f.good();) {
      std::string rest;
      std::getline(f,rest);
      Arc::trim(rest," \t\r\n");
      std::string name = "";
      for(;rest.length() != 0;) {
        name=Arc::ConfigIni::NextArg(rest,' ','"');
      };
      if(name.length() == 0) continue;
      for(std::list<std::string>::iterator u = ulist.begin();
                            u != ulist.end(); ++u) {
        if(name == *u) { name.resize(0); break; };
      };
      if(name.length() == 0) continue;
      ulist.push_back(name);
    };
    f.close();
    return true;
  }
  */

} // namespace gridftpd
