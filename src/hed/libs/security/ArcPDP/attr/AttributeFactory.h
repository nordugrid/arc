#ifndef __ARC_ATTRIBUTEFACTORY_H__
#define __ARC_ATTRIBUTEFACTORY_H__

#include <map>
#include <arc/XMLNode.h>
#include <arc/Logger.h>

#include <arc/loader/LoadableClass.h>

#include "AttributeProxy.h"

namespace Arc {

typedef std::map<std::string, Arc::AttributeProxy*> AttrProxyMap;

/** Base attribute factory class*/
class AttributeFactory : public LoadableClass {
public:
  AttributeFactory() {};
  virtual ~AttributeFactory(){};

public:
  virtual AttributeValue* createValue(const XMLNode& node, const std::string& type){};

protected:
  AttrProxyMap apmap;
};

} // namespace Arc

#endif /* __ARC_ATTRIBUTEFACTORY_H__ */

