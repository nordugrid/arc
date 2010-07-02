#ifndef INDEX_H
#define INDEX_H

#include <list>
#include <string>

#include "Policy.h"
#include "Entry.h"

class Index {
 public:
  Index(const std::string& name);
  Index(const Index& ix);
  ~Index();
  const std::string& Name() const;
  void ListEntries(FILE *f);
  bool AddEntry(const Entry& entry);
  void AllowReg(const std::string& areg);
 private:
  const std::string name;
  std::list<Policy> policies;
  std::list<Entry> entries;
  pthread_mutex_t lock;
};

#endif // INDEX_H
