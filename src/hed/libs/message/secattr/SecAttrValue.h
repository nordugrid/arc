#ifndef __ARC_SECATTRVALUE__
#define __ARC_SECATTRVALUE__
namespace Arc {
///This is an abstract interface to a security attribute
/**This class is meant to be inherited to implement security attributes. Depending on what data it needs to store inheriting classes may need to implement constructor and destructor. They must however override the equality and the boolean operators.
The equality is meant to compare security attributes. The prototype implies that all attributes are comparable to all others. This behaviour should be modified as needed by using dynamic_cast operations.
The boolean cast operation is meant to embody "nullness" if that is applicable to the particular type.
*/
  class SecAttrValue {
   public:
    SecAttrValue () {};
    virtual ~SecAttrValue () {};
    /**This function should (in inheriting classes) return true if this and b are considered to be the same. Identifying and restricting the type of b should be done using dynamic_cast operations.*/
    bool operator== (SecAttrValue &b);
    /**This is a convenience function to allow the usage of "not equal" conditions and need not be overridden.*/
    bool operator!= (SecAttrValue &b);
    /**This function should return false if the value is to be considered null, e g if it hasn't been set or initialized. In other cases it should return true.*/
    virtual operator bool ();
   protected:
    virtual bool equal (SecAttrValue &b);
  };
}
#endif
