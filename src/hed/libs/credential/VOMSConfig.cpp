// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string>
#include <fstream>

#include <arc/FileUtils.h>
#include <arc/StringConv.h>

#include "Credential.h"

#include "VOMSConfig.h"

namespace Arc {

VOMSConfigLine::operator bool(void) { return !name.empty(); }

bool VOMSConfigLine::operator!(void) { return name.empty(); }

const std::string& VOMSConfigLine::Name() const { return name; }

const std::string& VOMSConfigLine::Host() const { return host; }

const std::string& VOMSConfigLine::Port() const { return port; }

const std::string& VOMSConfigLine::Subject() const { return subject; }

const std::string& VOMSConfigLine::Alias() const { return alias; }

std::string VOMSConfigLine::Str() const {
  return "\"" + name + "\" \"" + host + "\" \"" + port + "\" \"" + subject + "\" \"" + alias + "\"";
}

VOMSConfig::iterator::operator bool(void) const { return (list_ && (*this != list_->end())); }

bool VOMSConfig::iterator::operator!(void) const { return (!list_ || (*this == list_->end())); }

VOMSConfig::iterator::iterator(void):
  std::list<VOMSConfigLine>::iterator(),
  list_(NULL)
{
}

VOMSConfig::iterator::iterator(const iterator& it):
  std::list<VOMSConfigLine>::iterator(it),
  list_(it.list_)
{
}

VOMSConfig::iterator& VOMSConfig::iterator::operator=(const VOMSConfig::iterator& it) {
  list_ = it.list_;
  std::list<VOMSConfigLine>::iterator::operator=((const std::list<VOMSConfigLine>::iterator&)it);
  return *this;
}

VOMSConfig::iterator::iterator(std::list<VOMSConfigLine>& list, std::list<VOMSConfigLine>::iterator it):
  std::list<VOMSConfigLine>::iterator(it),
  list_(&list)
{
}

bool VOMSConfig::filter::match(const VOMSConfigLine& line) const { return true; }

VOMSConfig::operator bool(void) const { return !lines.empty(); }

bool VOMSConfig::operator!(void) const { return lines.empty(); }

VOMSConfig::iterator VOMSConfig::FirstByName(const std::string name) {
  for(std::list<VOMSConfigLine>::iterator line = lines.begin(); line != lines.end(); ++line) {
    if(line->Name() == name) {
      return iterator(lines,line);
    };
  };
  return iterator(lines,lines.end());
}

VOMSConfig::iterator VOMSConfig::FirstByAlias(const std::string alias) {
  for(std::list<VOMSConfigLine>::iterator line = lines.begin(); line != lines.end(); ++line) {
    if(line->Alias() == alias) {
      return iterator(lines,line);
    };
  };
  return iterator(lines,lines.end());
}

VOMSConfig::iterator VOMSConfig::First(const VOMSConfig::filter& lfilter) {
  for(std::list<VOMSConfigLine>::iterator line = lines.begin(); line != lines.end(); ++line) {
    if(lfilter.match(*line)) {
      return iterator(lines,line);
    };
  };
  return iterator(lines,lines.end());
}

VOMSConfig::iterator VOMSConfig::iterator::NextByName(void) {
  if(!this->list_) return iterator();
  for(std::list<VOMSConfigLine>::iterator line = *this; ++line != this->list_->end(); ) {
    if(line->Name() == (*this)->Name()) {
      return iterator(*(this->list_),line);
    };
  };
  return iterator(*(this->list_),this->list_->end());
}

VOMSConfig::iterator VOMSConfig::iterator::NextByAlias(void) {
  if(!this->list_) return iterator();
  for(std::list<VOMSConfigLine>::iterator line = *this; ++line != this->list_->end(); ) {
    if(line->Alias() == (*this)->Alias()) {
      return iterator(*(this->list_),line);
    };
  };
  return iterator(*(this->list_),this->list_->end());
}

VOMSConfig::iterator VOMSConfig::iterator::Next(const VOMSConfig::filter& lfilter) {
  if(!this->list_) return iterator();
  for(std::list<VOMSConfigLine>::iterator line = *this; ++line != this->list_->end(); ) {
    if(lfilter.match(*line)) {
      return iterator(*(this->list_),line);
    };
  };
  return iterator(*(this->list_),this->list_->end());
}

VOMSConfigLine* VOMSConfig::iterator::operator->(void) const {
  if(!this->list_) return NULL;
  return std::list<VOMSConfigLine>::iterator::operator->();
}

VOMSConfigLine::VOMSConfigLine(const std::string& line) {
  std::string::size_type p = line.find_first_not_of("\t ");
  if(p == std::string::npos) return;
  if(line[p] == '#') return;
  std::vector<std::string> tokens;
  Arc::tokenize(line, tokens, " \t", "\"", "\"");
  // Normally there must be 5 items in line. 
  // But older voms files (still in use) have 6 parameters.
  if((tokens.size() != 5) && (tokens.size() != 6)) {
    CredentialLogger.msg(ERROR,"ERROR: VOMS configuration line contains too many tokens. Expecting 5 or 6. Line was: %s", line);
    return;
  };
  name    = tokens[0];
  host    = tokens[1];
  port    = tokens[2];
  subject = tokens[3];
  alias   = tokens[4];
}

bool VOMSConfig::AddPath(const std::string& path, int depth, const filter& lfilter) {
  const int max_line_length = 1024;
  const int max_lines_num = 1024;
  const int max_depth = 64;
  if(Glib::file_test(path, Glib::FILE_TEST_IS_DIR)) {
    if((++depth) >= max_depth) {
      CredentialLogger.msg(ERROR,"ERROR: file tree is too deep while scanning VOMS configuration. Max allowed nesting is %i.",max_depth);
      return false;
    };
    Glib::Dir dir(path);
    bool r = true;
    while(true) {
      std::string name = dir.read_name();
      if(name.empty()) break;
      if(!AddPath(Glib::build_filename(path,name),depth,lfilter)) r = false;
    };
    return r;
  } else if(Glib::file_test(path, Glib::FILE_TEST_IS_REGULAR)) {
    // Sanity check
    // Read and parse
    std::ifstream iv(path.c_str());
    int n = 0;
    while(!iv.eof()) {
      if(iv.fail()) {
        CredentialLogger.msg(ERROR,"ERROR: failed to read file %s while scanning VOMS configuration.",path);
        return false;
      };
      if((n++) >= max_lines_num) {
        // too many lines
        CredentialLogger.msg(ERROR,"ERROR: VOMS configuration file %s contains too many lines. Max supported number is %i.",path,max_lines_num);
        return false;
      };
      char buf[max_line_length];
      iv.getline(buf,sizeof(buf));
      if(iv.gcount() >= (sizeof(buf)-1)) {
        // too long line
        CredentialLogger.msg(ERROR,"ERROR: VOMS configuration file %s contains too long line(s). Max supported length is %i characters.",path,max_line_length-1);
        return false;
      };
      VOMSConfigLine vline(buf);
      if(vline && lfilter.match(vline)) lines.push_back(vline);
    };
    return true;
  };
  // This is not error. Just file we can't process. So skip logging.
  return false;
}

VOMSConfig::VOMSConfig(const std::string& path, const VOMSConfig::filter& lfilter) {
  AddPath(path,0,lfilter);
}

} //namespace Arc

