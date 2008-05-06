#ifndef __ARC_SEC_DATETIMEATTRIBUTE_H__
#define __ARC_SEC_DATETIMEATTRIBUTE_H__

#include <arc/security/ArcPDP/attr/AttributeValue.h>
#include <arc/DateTime.h>

namespace ArcSec {
//DateTimeAttribute, TimeAttribute, DateAttribute, DurationAttribute, PeriodAttribute
//As reference: See http://www.ietf.org/rfc/rfc3339.txt

//DateTimeAttribute
//Format: 
// YYYYMMDDHHMMSSZ
// Day Month DD HH:MM:SS YYYY
// YYYY-MM-DD HH:MM:SS
// YYYY-MM-DDTHH:MM:SS+HH:MM 
// YYYY-MM-DDTHH:MM:SSZ
class DateTimeAttribute : public AttributeValue {
private:
  static std::string identifier;
  Arc::Time value; //using the Time class definition in DateTime.h
  //std::string type;
  std::string id;

public:
  DateTimeAttribute() /*: type(identifier)*/{ };
  DateTimeAttribute(const std::string& v,const std::string& i) : value(v), /*type(identifier),*/ id(i) {};
  virtual ~DateTimeAttribute(){ };

  virtual bool equal(AttributeValue* other);
  virtual bool lessthan(AttributeValue* other);
  virtual bool inrange(AttributeValue* other);
  virtual std::string encode(); //encode value into ISOTime format
  Arc::Time getValue(){ return value; };
  virtual std::string getType() {return identifier; /*type;*/};
  virtual std::string getId() {return id;};
  static const std::string& getIdentifier(void) { return identifier; };
};

//TimeAttribute
//Format: 
// HHMMSSZ
// HH:MM:SS
// HH:MM:SS+HH:MM
// HH:MM:SSZ
class TimeAttribute : public AttributeValue {
private:
  static std::string identifier;
  Arc::Time value;
  //std::string type;
  std::string id;

public:
  TimeAttribute() /*: type(identifier)*/{ };
  TimeAttribute(const std::string& v,const std::string& i);
  virtual ~TimeAttribute(){ };

  virtual bool equal(AttributeValue* other);
  virtual bool lessthan(AttributeValue* other);
  virtual std::string encode();
  Arc::Time getValue(){ return value; };
  virtual std::string getType() {return identifier; /*type;*/};
  virtual std::string getId() {return id;};
  static const std::string& getIdentifier(void) { return identifier; };
};

//DateAttribute
//Formate: 
//YYYY-MM-DD
class DateAttribute : public AttributeValue {
private:
  static std::string identifier;
  Arc::Time value;
  //std::string type;
  std::string id;

public:
  DateAttribute() /*: type(identifier)*/{ };
  DateAttribute(const std::string& v,const std::string& i);
  virtual ~DateAttribute(){ };

  virtual bool equal(AttributeValue* other);
  virtual bool lessthan(AttributeValue* other);
  virtual std::string encode();
  Arc::Time getValue(){ return value; };
  virtual std::string getType() {return identifier; /*type;*/};
  virtual std::string getId() {return id;};
  static const std::string& getIdentifier(void) { return identifier; };
};


//DurationAttribute
//Formate: 
//P??Y??M??DT??H??M??S
class DurationAttribute : public AttributeValue {
private:
  static std::string identifier;
  Arc::Period value;
  //std::string type;
  std::string id;

public:
  DurationAttribute() /*: type(identifier)*/{ };
  DurationAttribute(const std::string& v,const std::string& i) : value(v), /*type(identifier),*/ id(i){};
  virtual ~DurationAttribute(){ };

  virtual bool equal(AttributeValue* other);
  virtual std::string encode();
  Arc::Period getValue(){ return value; };
  virtual std::string getType() {return identifier; /*type;*/};
  virtual std::string getId() {return id;};
  static const std::string& getIdentifier(void) { return identifier; };
};


//PeriodAttribute
//Formate: 
//datetime"/"duration
//datetime"/"datetime
//duration"/"datetime 
typedef struct{
  Arc::Time starttime;
  Arc::Time endtime;
  Arc::Period duration;
}ArcPeriod;

class PeriodAttribute : public AttributeValue {
private:
  static std::string identifier;
  ArcPeriod value;
  //std::string type;
  std::string id;

public:
  PeriodAttribute() /*: type(identifier)*/{ };
  PeriodAttribute(const std::string& v,const std::string& i);
  virtual ~PeriodAttribute(){ };

  virtual bool equal(AttributeValue* other);
  virtual std::string encode();
  ArcPeriod getValue(){ return value; };
  virtual std::string getType() {return identifier; /*type;*/};
  virtual std::string getId() {return id;};
  static const std::string& getIdentifier(void) { return identifier; };
};

}// namespace ArcSec

#endif /* __ARC_SEC_DATETIMEATTRIBUTE_H__ */

