#ifndef __ARC_SEC_ATTRIBUTEFACTORY_H__
#define __ARC_SEC_ATTRIBUTEFACTORY_H__

#include <map>
#include <arc/XMLNode.h>
#include <arc/Logger.h>
#include <arc/loader/LoadableClass.h>

#include "AttributeProxy.h"

namespace ArcSec {

typedef std::map<std::string, AttributeProxy*> AttrProxyMap;

/** Base attribute factory class*/
class AttributeFactory : public Arc::LoadableClass {
public:
  AttributeFactory() {};
  virtual ~AttributeFactory(){};

public:
  virtual AttributeValue* createValue(const Arc::XMLNode& node, const std::string& type) = 0;

protected:
  AttrProxyMap apmap;
};

} // namespace ArcSec

#endif /* __ARC_SEC_ATTRIBUTEFACTORY_H__ */

