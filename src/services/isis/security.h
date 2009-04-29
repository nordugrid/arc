#include <arc/message/SecAttr.h>

namespace ISIS {

// Id: http://www.nordugrid.org/schemas/policy-arc/types/isis/operation
// Value: isis - actions for inter-isis communication
//        service - actions for services registering to ISIS
//        client - actions for clients asking about information from ISIS

class ISISSecAttr: public Arc::SecAttr {
 public:
  ISISSecAttr(const std::string& action);
  virtual ~ISISSecAttr(void);
  virtual operator bool(void) const;
  virtual bool Export(Arc::SecAttrFormat format,Arc::XMLNode &val) const;
 protected:
  std::string action_;
  std::string object_;
  std::string context_;
  virtual bool equal(const Arc::SecAttr &b) const;
};

} // namespace ISIS

