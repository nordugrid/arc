#ifndef __ARC_DATETIMEATTRIBUTE_H__
#define __ARC_DATETIMEATTRIBUTE_H__

#include "AttributeValue.h"
#include "common/DateTime.h"

namespace Arc {
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
  Time value; //using the Time class definition in DateTime.h

public:
  DateTimeAttribute(){ };
  DateTimeAttribute(std::string v) : value(v){};
  virtual ~DateTimeAttribute(){ };

  virtual bool equal(AttributeValue* other);
  virtual std::string encode(); //encode value into ISOTime format
  Time getValue(){ return value; };
  static const std::string& identify(void) { return identifier; };

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
  Time value;

public:
  TimeAttribute(){ };
  TimeAttribute(std::string v);
  virtual ~TimeAttribute(){ };

  virtual bool equal(AttributeValue* other);
  virtual std::string encode();
  Time getValue(){ return value; };
  static const std::string& identify(void) { return identifier; };

};

//DateAttribute
//Formate: 
//YYYY-MM-DD
class DateAttribute : public AttributeValue {
private:
  static std::string identifier;
  Time value;

public:
  DateAttribute(){ };
  DateAttribute(std::string v);
  virtual ~DateAttribute(){ };

  virtual bool equal(AttributeValue* other);
  virtual std::string encode();
  Time getValue(){ return value; };
  static const std::string& identify(void) { return identifier; };
};


//DurationAttribute
//Formate: 
//P??Y??M??DT??H??M??S
class DurationAttribute : public AttributeValue {
private:
  static std::string identifier;
  Period value;

public:
  DurationAttribute(){ };
  DurationAttribute(std::string v) : value(v){};
  virtual ~DurationAttribute(){ };

  virtual bool equal(AttributeValue* other);
  virtual std::string encode();
  Period getValue(){ return value; };
  static const std::string& identify(void) { return identifier; };
};


//PeriodAttribute
//Formate: 
//datetime"/"duration
//datetime"/"datetime
//duration"/"datetime 
typedef struct{
  Time starttime;
  Time endtime;
  Period duration;
}ArcPeriod;

class PeriodAttribute : public AttributeValue {
private:
  static std::string identifier;
  ArcPeriod value;

public:
  PeriodAttribute(){ };
  PeriodAttribute(std::string v);
  virtual ~PeriodAttribute(){ };

  virtual bool equal(AttributeValue* other);
  virtual std::string encode();
  ArcPeriod getValue(){ return value; };
  static const std::string& identify(void) { return identifier; };
};

}// namespace Arc

#endif /* __ARC_DATETIMEATTRIBUTE_H__ */

