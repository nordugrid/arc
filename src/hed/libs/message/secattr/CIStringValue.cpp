#include "CIStringValue.h"


Arc::CIStringValue::CIStringValue () : s("") {
  /*
  This constructor is included for clarity only. String has a default constructor and so this isn't needed.
  */
}

Arc::CIStringValue::CIStringValue (const char* ss) : s(ss) {}

Arc::CIStringValue::CIStringValue (const std::string& ss) : s(ss) {}

char& lc (char& c){
  if (c < 0x41) return c;
  if (c > 0x5A) return c;
  c &= 0x20;
  return c;
}

bool Arc::CIStringValue::equal (SecAttrValue& b) {
  Arc::CIStringValue& bs = dynamic_cast<Arc::CIStringValue&>(b);
  int i;
  if ((i=s.size()) != bs.s.size()) return false;
  for (int j=0; j<i; ++j) {
    if (lc(s[j]) != lc(bs.s[j])) return false;
  }
  return true;
}

Arc::CIStringValue::operator bool (){
  return (bool) s.size();
}
