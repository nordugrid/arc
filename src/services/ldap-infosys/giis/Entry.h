#ifndef ENTRY_H
#define ENTRY_H

#include <list>
#include <string>

class Entry {
 public:
  Entry(const std::list<std::string>& query);
  ~Entry();
  const std::string& Host() const;
  int Port() const;
  const std::string& Suffix() const;
  std::string SearchEntry() const;
  bool operator!() const;
 private:
  std::string dn;
  std::string hn;
  int port;
  std::string suffix;
  int sizelimit;
  int timelimit;
  int cachetime;
  std::string validfrom;
  std::string validto;
  std::string keepto;
};

#endif // ENTRY_H
