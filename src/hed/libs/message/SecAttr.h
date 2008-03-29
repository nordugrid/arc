#ifndef __ARC_SECATTR__
#define __ARC_SECATTR__

#include <string>
#include <list>

#include <arc/XMLNode.h>

namespace Arc {
  /// This is an abstract interface to a security attribute
  /** This class is meant to be inherited to implement security attributes. 
     Depending on what data it needs to store inheriting classes may need to 
     implement constructor and destructor. They must however override the 
     equality and the boolean operators.
      The equality is meant to compare security attributes. The prototype 
     implies that all attributes are comparable to all others. This behaviour 
     should be modified as needed by using dynamic_cast operations.
      The boolean cast operation is meant to embody "nullness" if that is 
     applicable to the particular type. */
  class SecAttr {
   public:

    /// Export/import format
    typedef enum {
      UNDEFINED, /// own serialization/deserialization format
      ARCAuth,   /// representation for ARC authorization policy
      XACML,     /// represenation for XACML policy
      SAML       /// suitable for inclusion into SAML structures
    } Format;

    SecAttr() {};
    virtual ~SecAttr() {};
    /** This function should (in inheriting classes) return true if this and b 
       are considered to represent same content. Identifying and restricting 
       the type of b should be done using dynamic_cast operations.*/
    bool operator==(const SecAttr &b) const { return equal(b); };
    /** This is a convenience function to allow the usage of "not equal" 
       conditions and need not be overridden.*/
    bool operator!=(const SecAttr &b) const { return !equal(b); };
    /** Checking if this attribute is covered by another */
    virtual bool IsSubsetOf(const SecAttr &b) const;
    /** Checking if this attribute covers another */
    virtual bool IsSupersetOf(const SecAttr &b) const;

    /** This function should return false if the value is to be considered 
       null, e g if it hasn't been set or initialized. In other cases it 
       should return true.*/
    virtual operator bool();

    /** Convert internal structure into specified format.
      Returns false if format is not supported/suitable for 
      this attribute. */
    virtual bool Export(Format format,std::string &val) const;
    virtual bool Export(Format format,XMLNode &val) const;

    /** Fills internal structure from external object of 
       specified format. Retrns false if failed to do. */
    virtual bool Import(Format format,const std::string &val);
    virtual bool Import(Format format,const XMLNode &val);

   protected:
    virtual bool equal(const SecAttr &b) const;
  };

  /// Container of multiple SecAttr attributes
  /** This class combines multiple attributes. It's export/import
     methods catenate results of underlying objects. */
  class MultiSecAttr: public SecAttr {
   public:
    MultiSecAttr() {};
    virtual ~MultiSecAttr() {};
    virtual bool IsSubsetOf(const SecAttr &b) const;
    virtual bool IsSupersetOf(const SecAttr &b) const;
    virtual operator bool();
    virtual bool Export(Format format,XMLNode &val) const;
    virtual bool Import(Format format,const XMLNode &val);
   protected:
    std::list<SecAttr*> attrs_;
    virtual bool equal(const SecAttr &b) const;
    virtual bool Add(Format format,XMLNode &val);
  };

}

#endif

