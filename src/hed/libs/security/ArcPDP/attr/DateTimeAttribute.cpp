#include <iostream>
#include "DateTimeAttribute.h"

namespace Arc {

//DateTimeAttribute
std::string DateTimeAttribute::identifier = "datetime";
bool DateTimeAttribute::equal(AttributeValue* o){
  DateTimeAttribute *other;
  try{
    other = dynamic_cast<DateTimeAttribute*>(o);
  } catch(std::exception&) { };
  if(other==NULL){
    std::cerr<<"not DateTimeAttribute"<<std::endl;
    return false;
  }
  if(value==(other->getValue()))
    return true;
  else
    return false;
}

bool DateTimeAttribute::lessthan(AttributeValue* o){
  DateTimeAttribute *other;
  try{
    other = dynamic_cast<DateTimeAttribute*>(o);
  } catch(std::exception&) { };
  if(other==NULL){
    std::cerr<<"not DateTimeAttribute"<<std::endl;
    return false;
  }
  if(value<(other->getValue()))
    return true;
  else
    return false;
}

bool DateTimeAttribute::inrange(AttributeValue* o){
  PeriodAttribute *other;
  try{
    other = dynamic_cast<PeriodAttribute*>(o);
  } catch(std::exception&) { };
  if(other==NULL){
    std::cerr<<"not PeriodAttribute"<<std::endl;
    return false;
  } 
  ArcPeriod period = other->getValue(); 

  Time st, et;
  st = period.starttime;
  et = period.endtime;
  if(period.starttime == Time(-1))
    st = period.endtime - period.duration;
  else if(period.endtime == Time(-1))
    et = period.starttime + period.duration;

  if((value>=st)&&(value<=et))
    return true;
  else
    return false;  
} 

std::string DateTimeAttribute::encode(){
  return(value.str(ISOTime));
}

//TimeAttribute
std::string TimeAttribute::identifier = "time";
TimeAttribute::TimeAttribute(std::string v){
  std::string v1 = "1970-01-01T" + v;
  Arc::DateTimeAttribute attr(v1);
  value = attr.getValue(); 
}

bool TimeAttribute::equal(AttributeValue* o){
  TimeAttribute *other;
  try{
    other = dynamic_cast<TimeAttribute*>(o);
  } catch(std::exception&) { };
  if(other==NULL){
    std::cerr<<"not TimeAttribute"<<std::endl;
    return false;
  }
  if(value==(other->getValue()))
    return true;
  else
    return false;
}

bool TimeAttribute::lessthan(AttributeValue* o){
  TimeAttribute *other;
  try{
    other = dynamic_cast<TimeAttribute*>(o);
  } catch(std::exception&) { };
  if(other==NULL){
    std::cerr<<"not TimeAttribute"<<std::endl;
    return false;
  }
  if(value<(other->getValue()))
    return true;
  else
    return false;
}

std::string TimeAttribute::encode(){
  std::string v;
  v = value.str(ISOTime);
  return(v.substr(11));
}

//DateAttribute
std::string DateAttribute::identifier = "date";
DateAttribute::DateAttribute(std::string v){
  std::string v1 = v + "T00:00:00+00:00";
  Arc::DateTimeAttribute attr(v1);
  value = attr.getValue();
}

bool DateAttribute::equal(AttributeValue* o){
  DateAttribute *other;
  try{
    other = dynamic_cast<DateAttribute*>(o);
  } catch(std::exception&) { };
  if(other==NULL){
    std::cerr<<"not DateAttribute"<<std::endl;
    return false;
  }
  if(value==(other->getValue()))
    return true;
  else
    return false;
}

bool DateAttribute::lessthan(AttributeValue* o){
  DateAttribute *other;
  try{
    other = dynamic_cast<DateAttribute*>(o);
  } catch(std::exception&) { };
  if(other==NULL){
    std::cerr<<"not DateAttribute"<<std::endl;
    return false;
  }
  if(value<(other->getValue()))
    return true;
  else
    return false;
}

std::string DateAttribute::encode(){
  std::string v;
  v = value.str(ISOTime);
  return(v.substr(0,9));
}

//DurationAttribute
std::string DurationAttribute::identifier = "duration";
bool DurationAttribute::equal(AttributeValue* o){
  DurationAttribute *other;
  try{
    other = dynamic_cast<DurationAttribute*>(o);
  } catch(std::exception&) { };
  if(other==NULL){
    std::cerr<<"not DurationAttribute"<<std::endl;
    return false;
  }
  if((value.GetPeriod())==((other->getValue()).GetPeriod()))
    return true;
  else
    return false;
}

std::string DurationAttribute::encode(){
  return(std::string(value));
}

//PeriodAttribute
std::string PeriodAttribute::identifier = "period";
PeriodAttribute::PeriodAttribute(std::string v){
  (value.starttime).SetTime(-1);
  (value.endtime).SetTime(-1);
  (value.duration).SetPeriod(0);

  std::string::size_type pos = v.find("/");
  std::string s1 = v.substr(0, pos);
  std::string s2 = v.substr(pos+1);

  Time t1 = Time(s1);
  Time t2 = Time(s2);
  Period d1 = Period(s1);
  Period d2 = Period(s2);
  if(t1.GetTime()!=-1){
    if(t2.GetTime()!=-1){
      value.starttime = t1;
      value.endtime = t2;
    }
    else if(d2.GetPeriod()!=0){
      value.starttime = t1;
      value.duration = d2;
    }
    else
      std::cerr<<"Invalid ISO period format!"<<std::endl;
  }
  else{
    if(d1.GetPeriod()!=0){
      if(t2.GetTime()!=-1){
        value.duration = d1;
        value.endtime = t2;
      }
      else
        std::cerr<<"Invalid ISO period format!"<<std::endl;
    }
    else 
      std::cerr<<"Invalid ISO period format!"<<std::endl;
  }
}

bool PeriodAttribute::equal(AttributeValue* o){
  PeriodAttribute *other;
  try{
    other = dynamic_cast<PeriodAttribute*>(o);
  } catch(std::exception&) { };
  if(other==NULL){
    std::cerr<<"not PeriodAttribute"<<std::endl;
    return false;
  }
  
  ArcPeriod oth = other->getValue();
  Time ls, le, os, oe;
  Period ld, od;
  ls = value.starttime;
  le = value.endtime;
  os = oth.starttime;
  oe = oth.endtime;
  ld = value.duration;
  od = oth.duration;

  if((ls!=Time(-1))&&(le==Time(-1)))
    le = ls + ld;
  else if((ls==Time(-1))&&(le!=Time(-1)))
    ls = le - ld;
  else if((ls==Time(-1))||(le==Time(-1)))
    return false;

  if((os!=Time(-1))&&(oe==Time(-1)))
    oe = os + od;
  else if((os==Time(-1))&&(oe!=Time(-1)))
    os = oe - od;
  else if((os==Time(-1))||(oe==Time(-1)))
    return false;
  
  //std::cout<<ls.str()<<(std::string)(ld)<<os.str()<<(std::string)(od)<<std::endl;

  if((ls==os)&&(le==oe))
    return true;
  else 
    return false;
}

std::string PeriodAttribute::encode(){
  std::string s1 = (std::string)(value.starttime);
  std::string s2 = (std::string)(value.endtime);
  std::string s3 = (std::string)(value.duration);

  if(value.starttime==Time(-1))
    return(s3+"/"+s2);
  else if(value.endtime==Time(-1))
    return(s1+"/"+s3);
  else
    return(s1+"/"+s2);
}

}  //namespace Arc
