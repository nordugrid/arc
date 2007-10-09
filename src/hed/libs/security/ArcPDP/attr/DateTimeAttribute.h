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
  std::string type;

public:
  DateTimeAttribute() : type(identifier){ };
  DateTimeAttribute(std::string v) : value(v), type(identifier) {};
  virtual ~DateTimeAttribute(){ };

  virtual bool equal(AttributeValue* other);
  virtual bool lessthan(AttributeValue* other);
  virtual bool inrange(AttributeValue* other);
  virtual std::string encode(); //encode value into ISOTime format
  Arc::Time getValue(){ return value; };
  virtual std::string getType() {return type;};
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
  std::string type;

public:
  TimeAttribute() : type(identifier){ };
  TimeAttribute(std::string v);
  virtual ~TimeAttribute(){ };

  virtual bool equal(AttributeValue* other);
  virtual bool lessthan(AttributeValue* other);
  virtual std::string encode();
  Arc::Time getValue(){ return value; };
  virtual std::string getType() {return type;};
  static const std::string& getIdentifier(void) { return identifier; };
};

//DateAttribute
//Formate: 
//YYYY-MM-DD
class DateAttribute : public AttributeValue {
private:
  static std::string identifier;
  Arc::Time value;
  std::string type;

public:
  DateAttribute() : type(identifier){ };
  DateAttribute(std::string v);
  virtual ~DateAttribute(){ };

  virtual bool equal(AttributeValue* other);
  virtual bool lessthan(AttributeValue* other);
  virtual std::string encode();
  Arc::Time getValue(){ return value; };
  virtual std::string getType() {return type;};
  static const std::string& getIdentifier(void) { return identifier; };
};


//DurationAttribute
//Formate: 
//P??Y??M??DT??H??M??S
class DurationAttribute : public AttributeValue {
private:
  static std::string identifier;
  Arc::Period value;
  std::string type;

public:
  DurationAttribute() : type(identifier){ };
  DurationAttribute(std::string v) : value(v), type(identifier){};
  virtual ~DurationAttribute(){ };

  virtual bool equal(AttributeValue* other);
  virtual std::string encode();
  Arc::Period getValue(){ return value; };
  virtual std::string getType() {return type;};
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
  std::string type;

public:
  PeriodAttribute() : type(identifier){ };
  PeriodAttribute(std::string v);
  virtual ~PeriodAttribute(){ };

  virtual bool equal(AttributeValue* other);
  virtual std::string encode();
  ArcPeriod getValue(){ return value; };
  virtual std::string getType() {return type;};
  static const std::string& getIdentifier(void) { return identifier; };
};

}// namespace ArcSec

#endif /* __ARC_SEC_DATETIMEATTRIBUTE_H__ */

