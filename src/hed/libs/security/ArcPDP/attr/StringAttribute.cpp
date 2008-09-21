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
  else
    return false;
}

bool StringAttribute::inrange(AttributeValue* o){
  StringAttribute *other;
  try{
    other = dynamic_cast<StringAttribute*>(o);
  } catch(std::exception&) { };
  if(other==NULL){
    //std::cerr<<"not StringAttribute"<<std::endl;
    return false;
  }
  if(id != other->id) return false;

  //if there is a few values in the policy side, e.g.
  //<Attribute AttributeId="urn:arc:fileoperation">read; write; delete</Attribute>
  std::string other_value = other->getValue();

  size_t p1, p2;
  std::string str, str1, str2;
  str = value;

  do {
    p1 = str.find_first_not_of(" ");
    p2 = str.find_first_of(";");
    if(p2!=std::string::npos)
      str1 = str.substr(p1,p2-p1);
    else str1 = str.substr(p1);
    
    size_t f1, f2;
    std::string o_str, o_str1, o_str2;
    o_str = other_value;
    bool match = false;
    do {
      f1 = o_str.find_first_not_of(" ");
      f2 = o_str.find_first_of(";");
      if(f2!=std::string::npos)
        o_str1 = o_str.substr(f1,f2-f1);
      else o_str1 = o_str.substr(f1);
      if((o_str1.compare(str1)) == 0) {
        match = true; break;
      }
      o_str2=o_str.substr(f2+1);
      o_str.clear(); o_str = o_str2; o_str2.clear();
    } while(f2!=std::string::npos);
    
    if(match == false) {return false;};

    str2=str.substr(p2+1);
    str.clear(); str = str2; str2.clear();

  } while(p2!=std::string::npos);

  return true;
}


} //namespace ArcSec
