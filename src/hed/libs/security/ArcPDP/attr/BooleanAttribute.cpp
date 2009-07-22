#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>
#include "BooleanAttribute.h"

namespace ArcSec {

std::string BooleanAttribute::identifier = "bool";

bool BooleanAttribute::equal(AttributeValue* o){
  BooleanAttribute *other;
  try{
    other = dynamic_cast<BooleanAttribute*>(o);
  } catch(std::exception&) { };
  if(other==NULL){
    //std::cerr<<"not BooleanAttribute"<<std::endl;
    return false;
  }
  //if(id != other->id) return false;
  if(value==(other->getValue())) 
    return true;
  else 
    return false;
}

} //namespace ArcSec
