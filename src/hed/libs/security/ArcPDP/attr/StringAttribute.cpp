#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>
#include "StringAttribute.h"

namespace Arc {

std::string StringAttribute::identifier = "string";

bool StringAttribute::equal(AttributeValue* o){
  StringAttribute *other;
  try{
    other = dynamic_cast<StringAttribute*>(o);
  } catch(std::exception&) { };
  if(other==NULL){
    std::cerr<<"not StringAttribute"<<std::endl;
    return false;
  }
  if((value.compare(other->getValue()))==0)
    return true;
  else 
    return false;
}

} //namespace Arc
