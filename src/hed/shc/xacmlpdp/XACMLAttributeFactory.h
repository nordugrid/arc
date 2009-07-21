#ifndef __ARC_SEC_XACMLATTRIBUTEFACTORY_H__
#define __ARC_SEC_XACMLATTRIBUTEFACTORY_H__

#include <list>
#include <fstream>
#include <arc/XMLNode.h>
#include <arc/Logger.h>
#include <arc/security/ArcPDP/attr/AttributeFactory.h>

namespace ArcSec {

/// Attribute factory class for XACML specified attributes
class XACMLAttributeFactory : public AttributeFactory {
public:
  XACMLAttributeFactory();
  virtual ~XACMLAttributeFactory();

public:
  /**creat a AttributeValue according to the value in the XML node and the type; It should be the caller 
  to release the AttributeValue Object*/
  virtual AttributeValue* createValue(const Arc::XMLNode& node, const std::string& type);

private:
  void initDatatypes();
};

Arc::Plugin* get_xacmlpdp_attr_factory (Arc::PluginArgument*);

} // namespace ArcSec

#endif /* __ARC_SEC_XACMLATTRIBUTEFACTORY_H__ */

