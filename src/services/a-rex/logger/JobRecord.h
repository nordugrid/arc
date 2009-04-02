#include <istream>

#include <arc/XMLNode.h>

namespace ARex {

class JobRecord: public Arc::XMLNode {
 private:
  bool valid;
  void set(std::istream& i);
 public:
  std::string url;
  operator bool(void) { return valid; };
  bool operator!(void) { return !valid; };
  JobRecord(std::istream& i);
  JobRecord(const std::string& s);
  JobRecord(XMLNode& xml);
  JobRecord(const JobRecord& j);
  ~JobRecord(void);
};

}

