#include "SecAttrValue.h"

bool Arc::SecAttrValue::equal (SecAttrValue &b) {
  return true;
}

Arc::SecAttrValue::operator bool () {
  return true;
}

bool Arc::SecAttrValue::operator!= (SecAttrValue &b) {
  if (equal(b)) {
    return false;
  } else {
    return true;
  }
}

bool Arc::SecAttrValue::operator== (SecAttrValue &b) {
  return equal(b);
}
