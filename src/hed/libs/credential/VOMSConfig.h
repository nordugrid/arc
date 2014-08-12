// -*- indent-tabs-mode: nil -*-
#ifndef __ARC_VOMSCONFIG_H__
#define __ARC_VOMSCONFIG_H__

#include <string>

namespace Arc {

class VOMSConfigLine {
 public:
  VOMSConfigLine(const std::string& line);
  operator bool(void);
  bool operator!(void);
  const std::string& Name() const;
  const std::string& Host() const;
  const std::string& Port() const;
  const std::string& Subject() const;
  const std::string& Alias() const;
  std::string Str() const;
 private:
  std::string name;
  std::string host;
  std::string port;
  std::string subject;
  std::string alias;
};

class VOMSConfig {
 public:
  class filter {
   public:
    virtual bool match(const VOMSConfigLine& line) const;
  };
  class iterator: private std::list<VOMSConfigLine>::iterator {
    friend class VOMSConfig;
    public:
     iterator NextByName(void);
     iterator NextByAlias(void);
     iterator Next(const VOMSConfig::filter& lfilter);
     iterator& operator=(const iterator& it);
     operator bool(void) const;
     bool operator!(void) const;
     iterator(void);
     iterator(const iterator& it);
     VOMSConfigLine* operator->() const;
    private:
     std::list<VOMSConfigLine>* list_;
     iterator(std::list<VOMSConfigLine>& list, std::list<VOMSConfigLine>::iterator it);
  };
  VOMSConfig(const std::string& path, const filter& lfilter = filter());
  operator bool(void) const;
  bool operator!(void) const;
  iterator FirstByName(const std::string name);
  iterator FirstByAlias(const std::string alias);
  iterator First(const filter& lfilter);
 private:
  std::list<VOMSConfigLine> lines;
  bool AddPath(const std::string& path, int depth, const filter& lfilter);
};

} // namespace Arc

#endif // __ARC_VOMSCONFIG_H__
