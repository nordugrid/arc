#include <iostream>
#include "StringAttribute.h"

using namespace Arc;

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
