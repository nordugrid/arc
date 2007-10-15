#include <ctype.h>
#include <arc/StringConv.h>

#include "LDIFtoXML.h"

namespace ARex {


static bool get_ldif_string(std::istream& ldif,std::string& str) {
  while(ldif) {
    getline(ldif,str);
    if(str.empty()) continue;
    if(str[0] == '#') continue;
    return true;
  };
  return false;
}

static void strtolower(std::string& str) {
  std::string::size_type l = str.length();
  char* s = (char*)(str.c_str());
  for(;l>0;--l,++s) *s=tolower(*s);
}

static void trim(std::string& str) {
  std::string::size_type first = str.find_first_not_of(' ');
  if(first == std::string::npos) {
    str.resize(0); return;
  };
  std::string::size_type last = str.find_last_not_of(' ');
  str=str.substr(first,last-first+1);
  return;
}

static bool split_ldif_path(const std::string& str,std::list<std::pair<std::string,std::string> >& path) {
  std::string::size_type cur = 0;
  while(true) {
    std::string::size_type p = str.find('=',cur);
    if(p == std::string::npos) return true;
    std::string name = str.substr(cur,p-cur);
    std::string::size_type e = str.find(',',p);
    if(e == std::string::npos) e = str.length();
    std::string val = str.substr(p+1,e-p-1);
    trim(name); trim(val);
    strtolower(name); strtolower(val);
    path.push_front(std::pair<std::string,std::string>(name,val));
    cur=e+1;
  };
  return false;
}

static bool compare_paths(const std::list<std::pair<std::string,std::string> >& path1,const std::list<std::pair<std::string,std::string> >& path2,int size) {
  std::list<std::pair<std::string,std::string> >::const_iterator i1 = path1.begin();
  std::list<std::pair<std::string,std::string> >::const_iterator i2 = path2.begin();
  for(;size>0;--size) {
    if((i1 == path1.end()) && (i2 == path2.end())) break;
    if(i1 == path1.end()) return false;
    if(i2 == path2.end()) return false;
    if(i1->first != i2->first) return false;
    if(i1->second != i2->second) return false;
    ++i1; ++i2;
  };
  return true;
}

static Arc::XMLNode path_to_XML(const std::list<std::pair<std::string,std::string> >& path,Arc::XMLNode node) {
  Arc::XMLNode cur = node;
  std::list<std::pair<std::string,std::string> >::const_iterator i = path.begin();
  for(;i!=path.end();++i) {
    Arc::XMLNode n = cur[i->first];
    Arc::XMLNode nn;
    for(int num = 0;;++num) {
      nn=n[num];
      if(!nn) break;
      if((std::string)(nn.Attribute("name")) == i->second) break;
    };
    if(!nn) {
      nn=cur.NewChild(i->first);
      nn.NewAttribute("name")=i->second;
    };
    cur=nn;
  };
  return cur;  
}

static void reduce_name(std::string& name,Arc::XMLNode x) {
  std::string::size_type p = std::string::npos;
  for(;;) {
    p=name.rfind('-',p);
    if(p == std::string::npos) break;
    std::string urn = "urn:"+name.substr(0,p);
    std::string prefix = x.NamespacePrefix(urn.c_str());
    if(!prefix.empty()) {
      name=prefix+":"+name.substr(p+1);
      break;
    };
    --p;
  };
}

static void reduce_names(Arc::XMLNode x) {
  if(x.Size() == 0) return;
  std::string name = x.Name();
  reduce_name(name,x);
  x.Name(name.c_str());
  for(int n = 0;;++n) {
    Arc::XMLNode x_ = x.Child(n);
    if(!x_) break;
    reduce_names(x_);
  };
}

static void reduce_prefix(std::string& prefix) {
  std::string::size_type p = 0;
  p=0;
  for(;p<prefix.length();) {
    std::string::size_type p_ = p;
    for(;p_<prefix.length();++p_) if(prefix[p_] != '-') break;
    if(p!=p_) prefix.replace(p,p_-p,"");
    p_=prefix.find('-',p);
    if(p_ == std::string::npos) {
      prefix.replace(p+1,std::string::npos,"");
    } else {
      prefix.replace(p+1,p_-p,"");
    };
    ++p;
  };
}

bool LDIFtoXML(std::istream& ldif,const std::string& ldif_base,Arc::XMLNode xml) {
  std::list<std::pair<std::string,std::string> > base;
  split_ldif_path(ldif_base,base);
  std::string str;
  if(!get_ldif_string(ldif,str)) return true;
  for(;;) { // LDIF processing loop
    for(;;) { // Looking for dn:
      if(strncasecmp(str.c_str(),"dn:",3) == 0) break;
      if(!get_ldif_string(ldif,str)) { reduce_names(xml); return true; };
    };
    str.replace(0,3,"");
    std::list<std::pair<std::string,std::string> > dn;
    split_ldif_path(str,dn);
    if(base.size() > dn.size()) continue; // Above base
    if(!compare_paths(base,dn,base.size())) continue; // Wrong base
    // Removing base
    for(int n = 0;n<base.size();++n) dn.erase(dn.begin());
    Arc::XMLNode x = path_to_XML(dn,xml);
    if(!x) return false;
    Arc::NS ns;
    for(;;) { // Reading ObjectClass elements
      if(!get_ldif_string(ldif,str)) { reduce_names(xml); return true; };
      if(strncasecmp(str.c_str(),"objectclass:",12) != 0) break;
      // Converting ObjectClass into namespace
      str.replace(0,12,"");
      trim(str);
      std::string prefix=str;
      reduce_prefix(prefix);
      for(int n = 0;;++n) {
        if(ns.find(prefix+Arc::tostring(n)) == ns.end()) {
          ns[prefix+Arc::tostring(n)]="urn:"+str;
          break;
        };
      };
    };
    x.Namespaces(ns);
    // Read name:value data
    for(;;) {
      if(strncasecmp(str.c_str(),"dn:",3) == 0) break;
      std::string::size_type p = str.find(':');
      std::string name;
      std::string value;
      if(p == std::string::npos) {
        name=str;
      } else {
        name=str.substr(0,p);
        value=str.substr(p+1);
      };
      trim(name); trim(value);
      reduce_name(name,x);
      x.NewChild(name)=value; // TODO: XML escaping for value
      if(!get_ldif_string(ldif,str)) { reduce_names(xml); return true; };
    };
  };
  return false;
}

} // namespace ARex

