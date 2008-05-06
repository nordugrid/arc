#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>
#include "GenericAttribute.h"

namespace ArcSec {

std::string GenericAttribute::identifier("");

bool GenericAttribute::equal(AttributeValue* o){
  if(!o) return false;
  if( (getType() == (o->getType())) && 
      (getId() == (o->getId())) &&
      (encode() == (o->encode())) ) return true;
  return false;
}

} //namespace ArcSec
