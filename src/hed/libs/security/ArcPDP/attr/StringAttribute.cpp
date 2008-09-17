#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>
#include "StringAttribute.h"

namespace ArcSec {

std::string StringAttribute::identifier = "string";

bool StringAttribute::equal(AttributeValue* o){
  StringAttribute *other;
  try{
    other = dynamic_cast<StringAttribute*>(o);
  } catch(std::exception&) { };
  if(other==NULL){
    //std::cerr<<"not StringAttribute"<<std::endl;
    return false;
  }
  if(id != other->id) return false;
  if((value.compare(other->getValue()))==0)
    return true;
  else {
  //if there is a few values in the policy side, e.g.
  //<Attribute AttributeId="urn:arc:fileoperation">read; write; delete</Attribute>
    std::string other_value= other->getValue();
    size_t f1, f2;
    std::string str, str1, str2;
    str = value;
    do {
      f1 = str.find_first_not_of(" ");
      f2 = str.find_first_of(";");
      if(f2!=std::string::npos)
        str1 = str.substr(f1,f2-f1);
      else str1 = str.substr(f1);
      if((str1.compare(other_value)) == 0)
        return true;
      str2=str.substr(f2+1);
      str.clear(); str = str2; str2.clear();
    } while(f2!=std::string::npos);
    return false;
  }
}

} //namespace ArcSec
