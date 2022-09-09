#include <arc/message/SecAttr.h>

namespace ArcMCCTLSSec {

class DelegationSecAttr: public Arc::SecAttr {
 public:
  DelegationSecAttr(void);
  DelegationSecAttr(const char* policy_str,int policy_size = -1);
  virtual ~DelegationSecAttr(void);
  virtual operator bool(void) const;
  virtual bool Export(Arc::SecAttrFormat format,Arc::XMLNode &val) const;
  virtual std::map< std::string,std::list<std::string> > getAll() const { return std::map< std::string,std::list<std::string> >(); };
 protected:
  Arc::XMLNode policy_doc_;
  virtual bool equal(const Arc::SecAttr &b) const;
};

class DelegationMultiSecAttr: public Arc::MultiSecAttr {
 public:
  DelegationMultiSecAttr(void);
  virtual ~DelegationMultiSecAttr(void);
  //virtual operator bool() const;
  virtual bool Export(Arc::SecAttrFormat format,Arc::XMLNode &val) const;
  bool Add(const char* policy_str,int policy_size = -1);
  virtual std::map< std::string,std::list<std::string> > getAll() const { return std::map< std::string,std::list<std::string> >(); };
};

}
