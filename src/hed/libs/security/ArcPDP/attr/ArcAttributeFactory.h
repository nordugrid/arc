#ifndef __ARC_ARCATTRIBUTEFACTORY_H__
#define __ARC_ARCATTRIBUTEFACTORY_H__

#include <list>
#include <fstream>
#include <arc/XMLNode.h>
#include <arc/Logger.h>
#include "AttributeFactory.h"

namespace Arc {

/** Attribute factory class for Arc specified attributes*/
class ArcAttributeFactory : public AttributeFactory {
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

