#ifndef POLICY_H
#define POLICY_H

#include <string>
#include <regex.h>

class Policy {
 public:
  Policy(const std::string &pstr);
  Policy(const Policy& p);
  Policy& operator=(const Policy& p);
  ~Policy();
  bool Check(const std::string &host, int port,
	     const std::string &suffix) const;
 private:
  void RegComp();
  std::string host;
  int port;
  std::string suffix;
  regex_t host_rx;
  regex_t suffix_rx;
};

#endif // POLICY_H
