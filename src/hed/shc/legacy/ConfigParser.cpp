#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/StringConv.h>

#include <ConfigParser.h>

namespace ArcSHCLegacy {

ConfigParser::ConfigParser(const std::string& filename, Arc::Logger& logger):logger_(logger) {
  if(filename.empty()) {
    logger_.msg(Arc::ERROR, "Configuration file not specified");
    return;
  };
  f_.open(filename.c_str());
  if(!f_) {
    logger_.msg(Arc::ERROR, "Configuration file can not be read");
    return;
  };
}

ConfigParser::~ConfigParser(void) {
}

static bool is_nested(const std::string& line) {
  if(line.length() < 2) return false;
  if(line[0] != '"') return false;
  if(line[line.length()-1] != '"') return false;
  std::string::size_type p = 1;
  for(;p<line.length()-1;) {
    for(;p<line.length()-1;++p) if(!::isspace(line[p])) break;
    if(p >= line.length()-1) break;
    if(line[p] == '"') {
      for(++p;p<line.length()-1;++p) {
        if(line[p] == '"') break;
      };
      if(p >= line.length()-1) return false;
      ++p;
    } else {
      for(++p;p<line.length()-1;++p) {
        if(isspace(line[p])) break;
        if(line[p] == '"') return false;
      };
      if(p >= line.length()-1) break;
    };
  };
  return true;
}

bool ConfigParser::Parse(void) {
  if(!f_) {
    logger_.msg(Arc::ERROR, "Configuration file can not be read");
    return false;
  };
  while(!f_.eof()) {
    if(!f_) {
      logger_.msg(Arc::ERROR, "Configuration file can not be read");
      return false;
    };
    std::string line;
    getline(f_,line);
    line = Arc::trim(line);
    if(line.empty()) continue;
    if(line[0] == '#') continue;
    if(line[0] == '[') {
      if(line.length() < 2) {
        logger_.msg(Arc::ERROR, "Configuration file is broken - block name is too short: %s",line);
        return false;
      };
      if(!block_id_.empty()) {
        if(!BlockEnd(block_id_,block_name_)) {
          return false;
        };
      };
      line = line.substr(1,line.length()-2);
      block_id_ = "";
      block_name_ = "";
      std::string::size_type ps = line.find('/');
      if(ps != std::string::npos) {
        block_name_ = Arc::trim(line.substr(ps+1));
        line.resize(ps);
      };
      line = Arc::trim(line);
      block_id_ = line;
      if(!BlockStart(block_id_,block_name_)) {
        return false;
      };
      continue;
    };
    std::string cmd;
    std::string::size_type p = line.find('=');
    if(p == std::string::npos) {
      cmd = Arc::trim(line);
      line = "";
    } else {
      cmd = Arc::trim(line.substr(0,p));
      line = Arc::trim(line.substr(p+1));
      if(is_nested(line)) line=line.substr(1,line.length()-2);
    };
    if(cmd == "name") {
      if(p != std::string::npos) {
        block_name_ = Arc::trim(Arc::trim(line),"\"");
      };
      continue;
    };
    if(!ConfigLine(block_id_,block_name_,cmd,line)) {
      return false;
    };
  };
  if(!block_id_.empty()) {
    if(!BlockEnd(block_id_,block_name_)) return false;
  };
  return true;
}


} // namespace ArcSHCLegacy

