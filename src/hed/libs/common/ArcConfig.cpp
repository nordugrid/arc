// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <fstream>
#include <iostream>
#include <string>

#include <glibmm/miscutils.h>

#include <arc/ArcLocation.h>
#include <arc/User.h>
#include "ArcConfig.h"

namespace Arc {

  Config::Config(const char *filename)
    : file_name_(filename) {
    ReadFromFile(filename);
  }

  Config::~Config(void) {
    // NOP
  }

  static void _print(XMLNode& node, int skip) {
    int n;
    for (n = 0; n < skip; n++)
      std::cout << " ";
    std::string content = (std::string)node;
    std::cout << "* " << node.Name() << "(" << node.Size() << ")"
              << " = " << content << std::endl;
    for (n = 0;; n++) {
      XMLNode _node = node.Child(n);
      if (!_node)
        break;
      _print(_node, skip + 2);
    }
  }

  void Config::print(void) {
    _print(*((XMLNode*)this), 0);
  }

  bool Config::parse(const char *filename) {
    file_name_ = filename;
    return ReadFromFile(filename);
  }

  Config::Config(long cfg_ptr_addr) {
    Config *cfg = (Config*)cfg_ptr_addr;
    cfg->New(*this);
  }

  Config::Config(const Config& cfg)
    : XMLNode() {
    cfg.New(*this);
    file_name_ = cfg.file_name_;
  }

  void Config::save(const char *filename) {
    std::string str;
    GetDoc(str);
    std::ofstream out(filename);
    out << str;
    out.close();
  }

  bool Config::elementtoenum(Arc::XMLNode pnode,const char* ename,int& val,const char* const opts[]) {
    std::string v = ename?pnode[ename]:pnode;
    if(v.empty()) return true; // default
    for(int n = 0;opts[n];++n) {
      if(v == opts[n]) { val = n; return true; };
    }; 
    return false;
  }

  bool Config::elementtobool(Arc::XMLNode pnode,const char* ename,bool& val) {
    std::string v = ename?pnode[ename]:pnode;
    if(v.empty()) return true; // default
    if((v == "true") || (v == "1")) {
      val=true;
      return true;
    };
    if((v == "false") || (v == "0")) {
      val=false;
      return true;
    };
    return false;
  }


  BaseConfig::BaseConfig() : plugin_paths(ArcLocation::GetPlugins()), defaultca(false) {}

  void BaseConfig::AddPluginsPath(const std::string& path) {
    plugin_paths.push_back(path);
  }

  XMLNode BaseConfig::MakeConfig(XMLNode cfg) const {
    XMLNode mm = cfg.NewChild("ModuleManager");
    for (std::list<std::string>::const_iterator p = plugin_paths.begin();
         p != plugin_paths.end(); ++p)
      mm.NewChild("Path") = *p;
    return mm;
  }

  void BaseConfig::AddCredential(const std::string& cred) {
    credential = cred;
  }

  void BaseConfig::AddPrivateKey(const std::string& path) {
    key = path;
  }

  void BaseConfig::AddCertificate(const std::string& path) {
    cert = path;
  }

  void BaseConfig::AddProxy(const std::string& path) {
    proxy = path;
  }

  void BaseConfig::AddCAFile(const std::string& path) {
    cafile = path;
  }

  void BaseConfig::AddCADir(const std::string& path) {
    cadir = path;
  }

  void BaseConfig::AddOToken(const std::string& token) {
    otoken = token;
  }

  void BaseConfig::SetDefaultCA(bool use_default) {
    defaultca = use_default;
  }

  void BaseConfig::AddOverlay(XMLNode cfg) {
    overlay.Destroy();
    cfg.New(overlay);
  }

  void BaseConfig::GetOverlay(std::string fname) {
    overlay.Destroy();
    if (fname.empty())
      return;
    overlay.ReadFromFile(fname);
  }

} // namespace Arc
