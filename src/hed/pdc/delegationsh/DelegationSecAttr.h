#include <arc/message/SecAttr.h>

namespace ArcSec {

class DelegationSecAttr: public Arc::SecAttr {
 public:
  DelegationSecAttr(void);
  DelegationSecAttr(const char* policy_str,int policy_size = -1);
  virtual ~DelegationSecAttr(void);
  virtual operator bool(void);
  virtual bool Export(Format format,Arc::XMLNode &val) const;
 protected:
  Arc::XMLNode policy_doc_;
  virtual bool equal(const Arc::SecAttr &b) const;
};

}
