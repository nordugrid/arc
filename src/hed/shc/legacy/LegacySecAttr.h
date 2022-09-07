#include <string>
#include <list>

#include <arc/Logger.h>
#include <arc/message/SecAttr.h>

namespace ArcSHCLegacy {

/**
 * Container for athorization evaluation result.
 * It stores authorized VOs, groups and VOMS+VO
 * attributes associated with groups.
 * TODO: Merge with AuthUser.
 */
class LegacySecAttr: public Arc::SecAttr {
 public:
  LegacySecAttr(Arc::Logger& logger);
  virtual ~LegacySecAttr(void);

  // Common interface
  virtual operator bool(void) const;
  virtual bool Export(Arc::SecAttrFormat format,Arc::XMLNode &val) const;
  virtual std::string get(const std::string& id) const;
  virtual std::list<std::string> getAll(const std::string& id) const;
  virtual std::map< std::string,std::list<std::string> > getAll() const;

  // Specific interface
  void AddGroup(const std::string& group,
                const std::list<std::string>& vo,
                const std::list<std::string>& voms,
                const std::list<std::string>& otokens);
  const std::list<std::string> GetGroups(void) const { return groups_; };
  const std::list<std::string>& GetGroupVO(const std::string& group) const;
  const std::list<std::string>& GetGroupVOMS(const std::string& group) const;
  const std::list<std::string>& GetGroupOtokens(const std::string& group) const;
  void AddVO(const std::string& vo) { VOs_.push_back(vo); };
  const std::list<std::string> GetVOs(void) const { return VOs_; };

 protected:
  Arc::Logger& logger_;
  std::list<std::string> groups_;
  std::list<std::string> VOs_;
  std::list< std::list<std::string> > groupsVO_; // synchronized with groups_
  std::list< std::list<std::string> > groupsVOMS_; // synchronized with groups_
  std::list< std::list<std::string> > groupsOtokens_; // synchronized with otokens_
  virtual bool equal(const SecAttr &b) const;
};

} // namespace ArcSHCLegacy

