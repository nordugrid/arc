#ifndef __ARC_SECATTR__
#define __ARC_SECATTR__

#include <string>
#include <list>
#include <cstring>

#include <arc/XMLNode.h>

namespace Arc {
  /// Export/import format.
  /** Format is identified by textual identity string. Class description
     includes basic formats only. That list may be extended. */
  class SecAttrFormat {
   private:
    const char* format_;
   public:
    inline SecAttrFormat(const SecAttrFormat& format):format_(format.format_) {};
    inline SecAttrFormat(const char* format = ""):format_(format) {};
    inline SecAttrFormat& operator=(SecAttrFormat format) { format_=format.format_; return *this; };
    inline SecAttrFormat& operator=(const char* format) { format_=format; return *this; };
    inline bool operator==(SecAttrFormat format) { return (strcmp(format_,format.format_) == 0); };
    inline bool operator==(const char* format) { return (strcmp(format_,format) == 0); };
    inline bool operator!=(SecAttrFormat format) { return (strcmp(format_,format.format_) != 0); };
    inline bool operator!=(const char* format) { return (strcmp(format_,format) != 0); };
  };
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
    static SecAttrFormat UNDEFINED; /// own serialization/deserialization format
    static SecAttrFormat ARCAuth;   /// representation for ARC authorization policy
    static SecAttrFormat XACML;     /// represenation for XACML policy
    static SecAttrFormat SAML;      /// suitable for inclusion into SAML structures
    static SecAttrFormat GACL;      /// representation for GACL policy
    SecAttr() {};
    virtual ~SecAttr() {};
    /** This function should (in inheriting classes) return true if this and b 
       are considered to represent same content. Identifying and restricting 
       the type of b should be done using dynamic_cast operations. Currently
       it is not defined how comparison methods to be used. Hence their 
       implementation is not required. */
    bool operator==(const SecAttr &b) const { return equal(b); };
    /** This is a convenience function to allow the usage of "not equal" 
       conditions and need not be overridden.*/
    bool operator!=(const SecAttr &b) const { return !equal(b); };

    /** This function should return false if the value is to be considered 
       null, e.g. if it hasn't been set or initialized. In other cases it 
       should return true.*/
    virtual operator bool() const;

    /** Convert internal structure into specified format.
      Returns false if format is not supported/suitable for 
      this attribute.  */
    virtual bool Export(SecAttrFormat format,std::string &val) const;
    /** Convert internal structure into specified format.
      Returns false if format is not supported/suitable for 
      this attribute. XML node referenced by @val is turned 
      into top level element of specified format. */
    virtual bool Export(SecAttrFormat format,XMLNode &val) const;

    /** Fills internal structure from external object of 
       specified format. Returns false if failed to do. 
       The usage pattern for this method is not defined and
       it is provided only to make class symmetric. Hence
       it's implementation is not required yet. */
    virtual bool Import(SecAttrFormat format,const std::string &val);
    virtual bool Import(SecAttrFormat format,XMLNode val);

    /** Access to specific item of the security attribute.
       If there are few items of same id the first one is presented.
       It is meant to be used for tightly coupled SecHandlers
       and provides more effective interface than Export. */
    virtual std::string get(const std::string& id) const;

    /** Access to specific items of the security attribute.
       This method returns all items which have id assigned.
       It is meant to be used for tightly coupled SecHandlers
       and provides more effective interface than Export. */
    virtual std::list<std::string> getAll(const std::string& id) const;

    /** Access to all items of the security attribute at once.
       This method returns all items and their ids assigned.
       It is meant to be used for tightly coupled SecHandlers
       and provides more effective interface than Export. */
    virtual std::map< std::string,std::list<std::string> > getAll() const;

   protected:
    virtual bool equal(const SecAttr &b) const;
  };

  /// Container of multiple SecAttr attributes
  /** This class combines multiple attributes. It's export/import
     methods catenate results of underlying objects. Primary meaning
     of this class is to serve as base for classes implementing 
     multi level hierarchical tree-like descriptions of user identity.
     It may also be used for collecting information of same source or 
     kind. Like all information extracted from X509 certificate. */
  class MultiSecAttr: public SecAttr {
   public:
    MultiSecAttr() {};
    virtual ~MultiSecAttr();
    virtual operator bool() const;
    virtual bool Export(SecAttrFormat format,XMLNode &val) const;
    virtual bool Import(SecAttrFormat format,XMLNode val);
   protected:
    std::list<SecAttr*> attrs_;
    virtual bool equal(const SecAttr &b) const;
    virtual bool Add(SecAttrFormat format,XMLNode &val);
  };

  

}

#endif

