#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>
#include "X500NameAttribute.h"

namespace ArcSec {

std::string X500NameAttribute::identifier = "x500Name";

bool X500NameAttribute::equal(AttributeValue* o){
  X500NameAttribute *other;
  try{
    other = dynamic_cast<X500NameAttribute*>(o);
  } catch(std::exception&) { };
  if(other==NULL){
    std::cerr<<"not X500NameAttribute"<<std::endl;
    return false;
  }
  if((value.compare(other->getValue()))==0)  //Now, deal with it the same as StringAttribute.  
    return true;
  else 
    return false;
}

} //namespace ArcSec
