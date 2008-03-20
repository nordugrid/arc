#ifndef __ARC_CISTRINGVALUE__
#define __ARC_CISTRINGVALUE__
#include "SecAttrValue.h"
#include <string>
namespace Arc {
///This class implements case insensitive strings as security attributes.
/**This is an example of how to inherit SecAttrValue. The class is meant to implement security attributes that are case insensitive strings.*/
  class CIStringValue : public SecAttrValue {
    public:
      /**Default constructor*/
      CIStringValue ();
      /**This is a constructor that takes a string litteral.*/
      CIStringValue (const char* ss);
      /**This is a constructor that takes a string object.*/
      CIStringValue (const std::string& ss);
      /**This function returns false if the string is empty or uninitialized*/
      virtual operator bool ();
    protected:
      std::string s;
      /**This function returns true if two strings are the same apart from letter case*/
      virtual bool equal (SecAttrValue& b);
  };
}
#endif
