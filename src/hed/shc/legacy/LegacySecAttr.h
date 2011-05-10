#include <string>
#include <list>

#include <arc/Logger.h>
#include <arc/message/SecAttr.h>

namespace ArcSHCLegacy {

class LegacySecAttr: public Arc::SecAttr {
 public:
  LegacySecAttr(Arc::Logger& logger);
  virtual ~LegacySecAttr(void);

  // Common interface
  virtual operator bool(void) const;
  virtual bool Export(Arc::SecAttrFormat format,Arc::XMLNode &val) const;
  virtual std::string get(const std::string& id) const;
  virtual std::list<std::string> getAll(const std::string& id) const;

  // Specific interface
  void AddGroup(const std::string& group) { groups_.push_back(group); };
  const std::list<std::string> GetGroups(void) const { return groups_; };
  void AddVO(const std::string& vo) { vos_.push_back(vo); };
  const std::list<std::string> GetVOs(void) const { return vos_; };

 protected:
  Arc::Logger& logger_;
  std::list<std::string> groups_;
  std::list<std::string> vos_;
  virtual bool equal(const SecAttr &b) const;
};

} // namespace ArcSHCLegacy

