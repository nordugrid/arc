#ifndef __ARC_ARCATTRIBUTEFACTORY_H__
#define __ARC_ARCATTRIBUTEFACTORY_H__

#include <list>
#include <fstream>
#include "../../../../libs/common/XMLNode.h"
#include "../../../../libs/common/Logger.h"

namespace Arc {

/** Attribute factory class for Arc specified attributes*/
class ArcAttributeFactory : public AttributeFactoty {
public:
  ArcAttributeFactory();
  virtual ~ArcAttributeFactory();

public:
  virtual AttributeValue* createValue(const XMLNode& node, const std::string& type);

private:
  void initDatatypes();
};

} // namespace Arc

#endif /* __ARC_ARCATTRIBUTEFACTORY_H__ */

