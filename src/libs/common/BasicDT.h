#ifndef __ARC_BASICDT_H__
#define __ARC_BASICDT_H__

#include <string>
#include <time.h>
#include <map>

/**Deal with the serilization and deserilization about basic datatype (Build-in datatype in "XML Schema Part 2: Datatypes Second Edition": http://www.w3.org/TR/xmlschema-2/)*/

namespace Arc {

typedef enum XSDTYPETag 
{ XSD_UNKNOWN=1, XSD_INT, XSD_FLOAT, XSD_STRING, XSD_LONG, XSD_SHORT, \
                XSD_BYTE, XSD_UNSIGNEDLONG, \
                XSD_BOOLEAN, XSD_UNSIGNEDINT, XSD_UNSIGNEDSHORT, \
                XSD_UNSIGNEDBYTE, \
                XSD_DOUBLE, XSD_DECIMAL, XSD_DURATION, \
                XSD_DATETIME, XSD_TIME, XSD_DATE, \
                XSD_GYEARMONTH, XSD_GYEAR, XSD_GMONTHDAY, XSD_GDAY, \
                XSD_GMONTH, XSD_HEXBINARY, \
                XSD_BASE64BINARY, XSD_ANYURI, XSD_QNAME,  XSD_NOTATION, \
                XSD_INTEGER, \
                XSD_ARRAY, USER_TYPE,  XSD_NMTOKEN, XSD_ANY, XSD_NONNEGATIVEINTEGER, \
                XSD_POSITIVEINTEGER, XSD_NONPOSITIVEINTEGER, XSD_NEGATIVEINTEGER, \
                XSD_NORMALIZEDSTRING, XSD_TOKEN, XSD_LANGUAGE, XSD_NAME, \
                XSD_NCNAME, XSD_ID, XSD_IDREF, XSD_IDREFS, XSD_ENTITY, \
                XSD_ENTITIES, XSD_NMTOKENS, ATTACHMENT \
} XSDTYPE;

//primitive datatype
typedef char* 	xsd__string;
typedef bool 	xsd__boolean;
typedef float	xsd__float;
typedef double	xsd__double;
typedef double	xsd__decimal;
typedef long	xsd__duration;
typedef struct tm  xsd__dateTime;
typedef struct tm  xsd__time;
typedef struct tm  xsd__date;
typedef struct tm  xsd__gYearMonth;
typedef struct tm  xsd__gYear;
typedef struct tm  xsd__gMonthDay;
typedef struct tm  xsd__gDay;
typedef struct tm  xsd__gMonth;
typedef char*	xsd__anyURI;
typedef char*   xsd__QName;
typedef char*	xsd__NOTATION;

//derived datatype
typedef char* 	xsd__normalizedString;
typedef char*	xsd__token;
typedef char*	xsd__language;
typedef char*	xsd__IDREFS;
typedef char*	xsd__ENTITIES;
typedef char*	xsd__NMTOKEN;
typedef char*	xsd__NMTOKENS;
typedef char*   xsd__Name;
typedef char*	xsd__NCName;
typedef char*	xsd__ID;
typedef char*	xsd__IDREF;
typedef char*	xsd__ENTITY;
typedef long long  	xsd__integer;
typedef long long  	xsd__nonPositiveInteger;
typedef long long 	xsd__negativeInteger;
typedef long long  	xsd__long;
typedef int  	xsd__int;
typedef short  	xsd__short;
typedef signed char  	xsd__byte;
typedef unsigned long long  	xsd__nonNegativeInteger;
typedef unsigned long long  	xsd__unsignedLong;
typedef unsigned int  		xsd__unsignedInt;
typedef unsigned short	xsd__unsignedShort;
typedef unsigned char  	xsd__unsignedByte;
typedef unsigned long long 	xsd__positiveInteger;

//two special primitive datatype
typedef struct{
  xsd__unsignedByte* buf;
  xsd__int size;
}xsd__hexBinary;
typedef struct{
  xsd__unsignedByte* buf;
  xsd__int size;
}xsd__base64Binary;


class BasicType {
  public:
    BasicType();
    virtual ~BasicType();
    virtual XSDTYPE getType()=0;
    virtual void* getValue()=0;
    char* serialize();
    void deserialize(const char* buf);
    char* serialize(const char* buf);
 
  //protect:
    char* m_buf;
};

class String : public BasicType{
public:
    String();
    ~String();
    String(const xsd__string buf);
    XSDTYPE getType();
    xsd__string getString();
    void* getValue();
 
    xsd__string deserialize(const char* buf);
    char* serialize(const xsd__string buf);
};

class Boolean : public BasicType{
public:
    Boolean();
    ~Boolean();
    Boolean(const xsd__boolean* buf);
    XSDTYPE getType();
    xsd__boolean* getBoolean();
    void* getValue();
 
    xsd__boolean* deserialize(const char* buf);
    char* serialize(const xsd__boolean* buf);
};

class Float : public BasicType{
public:
    Float();
    ~Float();
    Float(const xsd__float* buf);
    XSDTYPE getType();
    xsd__float* getFloat();
    void* getValue();
 
    xsd__float* deserialize(const char* buf);
    char* serialize(const xsd__float* buf);
};

class Double : public BasicType{
public:
    Double();
    ~Double();
    Double(const xsd__double* buf);
    XSDTYPE getType();
    xsd__double* getDouble();
    void* getValue();
 
    xsd__double* deserialize(const char* buf);
    char* serialize(const xsd__double* buf);
};

class Decimal : public BasicType{
public:
    Decimal();
    ~Decimal();
    Decimal(const xsd__decimal* buf);
    XSDTYPE getType();
    xsd__decimal* getDecimal();
    void* getValue();
   
    xsd__decimal* deserialize(const char* buf);
    char* serialize(const xsd__decimal* buf);
};

class Duration : public BasicType{
public:
    Duration();
    ~Duration();
    Duration(const xsd__duration* buf);
    XSDTYPE getType();
    xsd__duration* getDuration();
    void* getValue();

    xsd__duration* deserialize(const char* buf);
    char* serialize(const xsd__duration* buf);
};

class DateTime : public BasicType{
public:
    DateTime();
    ~DateTime();
    DateTime(const xsd__dateTime* buf);
    XSDTYPE getType();
    xsd__dateTime* getDateTime();
    void* getValue();

    xsd__dateTime* deserialize(const char* buf);
    char* serialize(const xsd__dateTime* buf);
};



class Time{};

class Date{};

class GYearMonth{};

class GYear{};

class GMonthDay{};

class GDay{};

class GMonth{};

class AnyURI : public BasicType{
public:
    AnyURI();
    ~AnyURI();
    AnyURI(const xsd__anyURI buf);
    XSDTYPE getType();
    xsd__anyURI getAnyURI();
    void* getValue();
  
    xsd__anyURI deserialize(const char* buf);
    char* serialize(const xsd__anyURI buf);
};

class QName : public BasicType{
public:
    QName();
    ~QName();
    QName(const xsd__QName buf);
    XSDTYPE getType();
    xsd__QName getQName();
    void* getValue();

    xsd__QName deserialize(const char* buf);
    char* serialize(const xsd__QName buf);
};

class NOTATION : public BasicType{
public:
    NOTATION();
    ~NOTATION();
    NOTATION(const xsd__NOTATION buf);
    XSDTYPE getType();
    xsd__NOTATION getNOTATION();
    void* getValue();

    xsd__NOTATION deserialize(const char* buf);
    char* serialize(const xsd__NOTATION buf);
};



class NormalizedString{};
class Token{};
class Language{};
class IDREFS{};
class ENTITIES{};
class NMTOKEN{};
class NMTOKENS{};
class Name{};
class NCName{};
class ID{};
class IDREF{};
class ENTITY{};

class Integer : public Decimal {
public:
    Integer();
    ~Integer();
    Integer(const xsd__integer buf);
    XSDTYPE getType();
    xsd__integer getInteger();
    void* getValue();

    xsd__integer deserialize(const char* buf);
    char* serialize(const xsd__integer buf);
};


class NonPositiveInteger : public Decimal {
public:
    NonPositiveInteger();
    ~NonPositiveInteger();
    NonPositiveInteger(const xsd__nonPositiveInteger buf);
    XSDTYPE getType();
    xsd__nonPositiveInteger getInteger();
    void* getValue();

    xsd__nonPositiveInteger deserialize(const char* buf);
    char* serialize(const xsd__nonPositiveInteger buf);
};

class NegativeInteger : public Decimal {
public:
    NegativeInteger();
    ~NegativeInteger();
    NegativeInteger(const xsd__negativeInteger buf);
    XSDTYPE getType();
    xsd__negativeInteger getNegativeInteger();
    void* getValue();

    xsd__negativeInteger deserialize(const char* buf);
    char* serialize(const xsd__negativeInteger buf);
};

class Long : public Decimal {
public:
    Long();
    ~Long();
    Long(const xsd__long buf);
    XSDTYPE getType();
    xsd__long getLong();
    void* getValue();

    xsd__long deserialize(const char* buf);
    char* serialize(const xsd__long buf);
};
/*
class Int : public Long {

}

class Short : public Int{
public:
    Short();
    ~Short();
}
*/
/*
class Byte{};
class NonNegativeInteger{};
class UnsignedLong{};
class UnsignedInt{};
class UnsignedShort{};
class UnsignedByte{};
class PositiveInteger{};
*/
/*
class HexBinary{};
class Base64Binary{};
*/
} // namespace Arc

#endif /* __ARC_BASICDT_H__ */
