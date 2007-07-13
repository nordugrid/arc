#include "BasicDT.h"
#include "StringConv.h"

namespace Arc {
/*****************************************/
BasicType::BasicType():m_buf(NULL){
}

BasicType::~BasicType(){
  if(m_buf){
    delete [] m_buf;
    m_buf = NULL;
  }
}

char* BasicType::serialize(){
  return m_buf;
}

void BasicType::deserialize(const char* buf){
  if(m_buf){
    delete [] m_buf;
    m_buf = NULL;
  }
  if(buf){
    m_buf = new char[strlen(buf)+1];
    strcpy(m_buf,buf);
  }
} 

char* BasicType::serialize(const char* buf){
  if(m_buf){
    delete [] m_buf;
    m_buf = NULL;
  } 
  m_buf = new char[strlen(buf)+1];
  strcpy(m_buf, buf);
  return m_buf;
}

/*****************************************/
String::String(){
}
String::String(const xsd__string buf){
  if(buf)
    serialize(buf);
}
String::~String(){
}
XSDTYPE String::getType(){
  return XSD_STRING;
}
xsd__string String::getString(){
  if(m_buf)
    return deserialize(m_buf);
  else
    return NULL;
}
void* String::getValue(){
  return (void*)getString();
}
char* String::serialize(const xsd__string buf){
  BasicType::serialize(buf);
  return m_buf;
}
xsd__string String::deserialize(const char* buf){
  xsd__string tmp = new char[strlen(buf)+1];
  strcpy (tmp, buf);
  return tmp; 
}

/*****************************************/
Boolean::Boolean(){
}

Boolean::~Boolean(){
}

Boolean::Boolean(const xsd__boolean* buf){
  if(buf)
    serialize(buf);
}

XSDTYPE Boolean::getType(){
  return XSD_BOOLEAN;
}

xsd__boolean* Boolean::getBoolean(){
  if(m_buf)
    return deserialize(m_buf);
  else
    return NULL;
}

void* Boolean::getValue(){
  return (void*)getBoolean();
}

char* Boolean::serialize(const xsd__boolean* buf){
  BasicType::serialize(*buf ? "true" : "false");
  return m_buf;
}

xsd__boolean * Boolean::deserialize(const char* buf){
  xsd__boolean * tmp = new xsd__boolean;
  if (0==strcmp(buf,"true") || 0==strcmp(buf,"1")){
    *tmp = 1;
  }
  else{
    *tmp=0;
  }
  return tmp;
}

/****************************************/
Float::Float(){
}

Float::Float(const xsd__float* buf){
  if(buf){
    serialize(buf);
  }
}

Float::~Float(){}

XSDTYPE Float::getType(){
  return XSD_FLOAT;
}

xsd__float* Float::getFloat(){
  if(m_buf!= NULL)
    return deserialize(m_buf);
  else
    return NULL;
}

void* Float::getValue(){
  return (void*)getFloat();
}

char* Float::serialize(const xsd__float* buf){
   BasicType::serialize(tostring(*buf, 0, 6).c_str());
   return m_buf;
}

xsd__float* Float::deserialize(const char* buf){
  xsd__float *val = new xsd__float(stringtof(buf));
  return val;
}

/****************************************/
Double::Double(){}
Double::Double(const xsd__double* buf){
  if(buf){
    serialize(buf);
  }
}

Double::~Double(){}

XSDTYPE Double::getType(){
  return XSD_DOUBLE;
}

xsd__double* Double::getDouble(){
  if(m_buf!= NULL)
    return deserialize(m_buf);
  else
    return NULL;
}

void* Double::getValue(){
  return (void*)getDouble();
}

char* Double::serialize(const xsd__double* buf){
   BasicType::serialize(tostring(*buf, 0, 10).c_str());
   return m_buf;
}

xsd__double* Double::deserialize(const char* buf){
  xsd__double *val = new xsd__double(stringtod(buf));
  return val;
}

/*****************************************/
Decimal::Decimal(){
}

Decimal::~Decimal(){
}

Decimal::Decimal(const xsd__decimal* buf)
{
  if(buf)
    serialize(buf);
}

XSDTYPE Decimal::getType(){
  return XSD_DECIMAL;
}

xsd__decimal* Decimal::getDecimal(){
  if(m_buf!= NULL)
    return deserialize(m_buf);
  else
    return NULL;
}

void* Decimal::getValue(){
  return (void*)getDecimal();
}

char* Decimal::serialize(const xsd__decimal* buf){
   BasicType::serialize(tostring(*buf).c_str());
   return m_buf;
}

xsd__decimal* Decimal::deserialize(const char* buf){
  xsd__decimal * val = new xsd__decimal(stringtod(buf));
  return val;
}

/****************************************/
Duration::Duration(){
}

Duration::~Duration(){
}

Duration::Duration(const xsd__duration* buf)
{
  if(buf)
    serialize(buf);
}

XSDTYPE Duration::getType(){
  return XSD_DURATION;
}

xsd__duration* Duration::getDuration(){
  if(m_buf!= NULL)
    return deserialize(m_buf);
  else
    return NULL;
}

void* Duration::getValue(){
  return (void*)getDuration();
}

char* Duration::serialize(const xsd__duration* buf){
  std::string serializedValue(*buf);
  BasicType::serialize(serializedValue.c_str());
  return m_buf;
}

xsd__duration* Duration::deserialize(const char* buf){
  xsd__duration* val = new xsd__duration(buf);
  return val;
}

/********************************************/
DateTime::DateTime(){
}

DateTime::~DateTime(){
}

DateTime::DateTime(const xsd__dateTime* buf)
{
  if(buf)
    serialize(buf);
}

XSDTYPE DateTime::getType(){
  return XSD_DATETIME;
}

xsd__dateTime* DateTime::getDateTime(){
  if(m_buf!= NULL)
    return deserialize(m_buf);
  else
    return NULL;
}

void* DateTime::getValue(){
  return (void*)getDateTime();
}

char* DateTime::serialize(const xsd__dateTime* buf){
  std::string serializedValue(buf->str(ISOTime));
  BasicType::serialize(serializedValue.c_str());
  return m_buf;
}

xsd__dateTime* DateTime::deserialize(const char* buf){
  xsd__dateTime *val = new xsd__dateTime(buf);
  return val;
}

/****************************************/


//xsd__time;
//xsd__date;
//xsd__gYearMonth;
//xsd__gYear;
//xsd__gMonthDay;
//xsd__gDay;
//xsd__gMonth;

/****************************************/
AnyURI::AnyURI(){
}
AnyURI::AnyURI(const xsd__string buf){
  if(buf)
    serialize(buf);
}
AnyURI::~AnyURI(){
}
XSDTYPE AnyURI::getType(){
  return XSD_ANYURI; 
}
xsd__anyURI AnyURI::getAnyURI(){
  if(m_buf) 
    return deserialize(m_buf);
  else
    return NULL;
} 
void* AnyURI::getValue(){
  return (void*)getAnyURI();
}
char* AnyURI::serialize(const xsd__anyURI buf){
  BasicType::serialize(buf);
  return m_buf;
} 
xsd__anyURI AnyURI::deserialize(const char* buf){
  xsd__anyURI tmp = new char[strlen(buf)+1];
  strcpy (tmp, buf);
  return tmp;
}

/****************************************/
QName::QName(){
}
QName::QName(const xsd__QName buf){
  if(buf)
    serialize(buf);
}
QName::~QName(){
}

XSDTYPE QName::getType(){
  return XSD_QNAME;
}
xsd__QName QName::getQName(){
  if(m_buf)
    return deserialize(m_buf);
  else
    return NULL;
}
void* QName::getValue(){
  return (void*)getQName();
}
char* QName::serialize(const xsd__QName buf){
  BasicType::serialize(buf);
  return m_buf;
}
xsd__QName QName::deserialize(const char* buf){
  xsd__QName tmp = new char[strlen(buf)+1];
  strcpy (tmp, buf);
  return tmp;
}

/******************************************/
NOTATION::NOTATION(){
}
NOTATION::NOTATION(const xsd__NOTATION buf){
  if(buf)
    serialize(buf);
}
NOTATION::~NOTATION(){
}

XSDTYPE NOTATION::getType(){
  return XSD_NOTATION;
}
xsd__NOTATION NOTATION::getNOTATION(){
  if(m_buf)
    return deserialize(m_buf);
  else
    return NULL;
}
void* NOTATION::getValue(){
  return (void*)getNOTATION();
}
char* NOTATION::serialize(const xsd__NOTATION buf){
  BasicType::serialize(buf);
  return m_buf;
}
xsd__NOTATION NOTATION::deserialize(const char* buf){
  xsd__NOTATION tmp = new char[strlen(buf)+1];
  strcpy (tmp, buf);
  return tmp;
}

} //  namespace Arc


/*********************************************/
/*class Integer::Integer(){
Integer::Integer(const xsd__integer buf){
  if(buf)
    serialize(buf);
}
Integer::~Integer(){
}

XSDTYPE Integer::getType(){
  return XSD_INTEGER;
}
xsd__Integer Integer::getInteger(){
  if(m_buf)
    return deserialize(m_buf);
  else
    return NULL;
}
void* Integer::getValue(){
  return (void*)getInteger();
}
char* Integer::serialize(const xsd__integer buf){
  BasicType::serialize(buf);
  return m_buf;
}
xsd__integer Integer::deserialize(const char* buf){
  xsd__integer tmp = new char[strlen(buf)+1];
  strcpy (tmp, buf);
  return tmp;
}
}


typedef long long       xsd__integer;
typedef long long       xsd__nonPositiveInteger;
typedef long long       xsd__negativeInteger;
typedef long long       xsd__long;
typedef int     xsd__int;
typedef short   xsd__short;
typedef signed char     xsd__byte;
typedef unsigned long long      xsd__nonNegativeInteger;
typedef unsigned long long      xsd__unsignedLong;
typedef unsigned int            xsd__unsignedInt;
typedef unsigned short  xsd__unsignedShort;
typedef unsigned char   xsd__unsignedByte;
typedef unsigned long long      xsd__positiveInteger;

//two special primitive datatype
typedef struct{
  xsd__unsignedByte* buf;
  xsd__int size;
}xsd__hexBinary;
typedef struct{
  xsd__unsignedByte* buf;
  xsd__int size;
}xsd__base64Binary;
*/

