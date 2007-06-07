#include "BasicDT.h"

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
}

void* Boolean::getValue(){
  return (void*)getBoolean();
}

char* Boolean::serialize(const xsd__boolean* buf){
  char * tmp = new char[6];
  sprintf(tmp, "%s", (*((int*)(buf))==0) ? "false" : "true");

  BasicType::serialize(tmp);
  delete [] tmp;
  //m_buf has been changed
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
  else return NULL;
}

void* Float::getValue(){
  return (void*)getFloat();
}

char* Float::serialize(const xsd__float* buf){
   char* tmp = new char[64];
   sprintf(tmp, "%.6g", *buf);

   BasicType::serialize(tmp);
   delete [] tmp;
   //m_buf has been changed
   return m_buf;
}

xsd__float* Float::deserialize(const char* buf){
  char* end;
  xsd__float * val = new xsd__float;
  *val = strtod (buf, &end);
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
  else return NULL;
}

void* Double::getValue(){
  return (void*)getDouble();
}

char* Double::serialize(const xsd__double* buf){
   char* tmp = new char[64];
   sprintf(tmp, "%.10g", *buf);

   BasicType::serialize(tmp);
   delete [] tmp;
   //m_buf has been changed
   return m_buf;
}

xsd__double* Double::deserialize(const char* buf){
  char* end;
  xsd__double * val = new xsd__double;
  *val = strtod (buf, &end);
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
  else return NULL;
}

void* Decimal::getValue(){
  return (void*)getDecimal();
}

char* Decimal::serialize(const xsd__decimal* buf){
   char* tmp = new char[64];
   sprintf(tmp, "%f", *buf);
	
   BasicType::serialize(tmp);
   delete [] tmp;        
   //m_buf has been changed
   return m_buf;
}

xsd__decimal* Decimal::deserialize(const char* buf){
  char* end;
  xsd__decimal * val = new xsd__decimal;
  *val = strtod (buf, &end);
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
  else return NULL;
}

void* Duration::getValue(){
  return (void*)getDuration();
}

char* Duration::serialize(const xsd__duration* buf){
  long value = *buf;
  char tmp[4];
  std::string serializedValue;
  //Duration's form:PnYnMnDTnHnMnS
  serializedValue = "P";
	    
  // Years
  int x = 365 * 24 * 3600;
  int Years = value/x;
  long tmpYears = Years * x;
  sprintf(tmp,"%d", Years);
  serializedValue.append (tmp);
  serializedValue.append ("Y");
  // Months
  value = value - (tmpYears);
  x = 30 * 24 * 3600;
  int Months = value / x;
  sprintf(tmp, "%d", Months);
  serializedValue.append (tmp);
  serializedValue.append ("M");
  // Days
  value = value - (Months * x);
  x = 24 * 3600;
  int Days = value / x;
  sprintf (tmp, "%d", Days);
  serializedValue.append (tmp);
  serializedValue.append ("DT");
  // Hours
  value = value - (Days * x);
  x = 3600;
  int Hours = value / x;
  sprintf (tmp, "%d", Hours);
  serializedValue.append (tmp);
  serializedValue.append ("H");
  // Minutes
  value = value - (Hours * x);
  x = 60;
  int Mins = value / x;
  sprintf (tmp, "%d", Mins);
  serializedValue.append (tmp);
  serializedValue.append ("M");
  //  Seconds
  int Secs = value - (Mins * x);
  sprintf(tmp, "%d", Secs);
  serializedValue.append (tmp);
  serializedValue.append ("S");
   
  char* temp = (char*) serializedValue.c_str();
  BasicType::serialize(temp);
  //m_buf has been changed
  return m_buf;
}

xsd__duration* Duration::deserialize(const char* buf){
  std::string value = buf;
  std::string tmp;
  unsigned int pos1, pos2, pos3, pos4, pos5, pos6;
  xsd__duration* deserializedValue = new xsd__duration;
	
  //XSD_DURATION format: PnYnMnDTnHnMnS
  // Years
  *deserializedValue = 0;
  pos1 = value.find_first_of("Y");
  tmp = value.substr (1, pos1-1);
  int years = atoi (tmp.c_str ());
  *deserializedValue += years * 365 * 24 * 3600;
  // Months
  pos2 = value.find_first_of ("M");
  tmp = value.substr (pos1+1, pos2-pos1-1);
  int months = atoi (tmp.c_str ());
  *deserializedValue += months * 30 * 24 * 3600;
  //Days
  pos3 = value.find_first_of ("D");
  tmp = value.substr (pos2+1, pos3-pos2-1);
  int days = atoi (tmp.c_str ());
  *deserializedValue += days * 24 * 3600;
  // Hours
  pos4 = value.find_first_of ("H");
  tmp = value.substr (pos3+2, pos4-pos3-2);
  int hours = atoi (tmp.c_str ());
  *deserializedValue += hours * 3600;
  // Minutes
  pos5 = value.find_last_of ("M");
  tmp = value.substr (pos4+1, pos5-pos4-1);
  int mins = atoi (tmp.c_str ());
  *deserializedValue += mins * 60;
  // Seconds
  pos6 = value.find_first_of ("S");
  tmp = value.substr (pos5+1, pos6-pos5-1);
  int secs = atoi (tmp.c_str ());
  *deserializedValue += secs;

  return deserializedValue;
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
  else return NULL;
}

void* DateTime::getValue(){
  return (void*)getDateTime();
}

char* DateTime::serialize(const xsd__dateTime* buf){
  std::string serializedValue = "";
  char* tmp = new char[64];
  strftime (tmp, 64, "%Y-%m-%dT%H:%M:%S", buf);
  serializedValue += tmp;
  delete [] tmp;
  //Get the local timezone offset
  time_t now = time(NULL);
  struct tm *temp = gmtime(&now);
  struct tm utcTime;
  memcpy(&utcTime, temp, sizeof(struct tm));
  temp = localtime(&now);
  struct tm localTime;
  memcpy(&localTime, temp, sizeof(struct tm));
  long utcTimeInMinutes = (utcTime.tm_year * 60 * 24 * 365)
      + (utcTime.tm_yday * 60 * 24)
      + (utcTime.tm_hour * 60)
      + utcTime.tm_min;
  long localTimeInMinutes = (localTime.tm_year * 60 * 24 * 365)
      + (localTime.tm_yday * 60 * 24)
      + (localTime.tm_hour * 60)
      + localTime.tm_min;
  int timeOffsetInMinutes = (int) (localTimeInMinutes - utcTimeInMinutes);
  if (timeOffsetInMinutes == 0){
    serializedValue += "Z";
  }
  else
  {
    struct tm timeOffset;
    timeOffset.tm_year = 0;
    timeOffset.tm_yday = 0;
    timeOffset.tm_sec = 0;
    timeOffset.tm_min = timeOffsetInMinutes % 60;
    timeOffsetInMinutes -= timeOffset.tm_min;
    timeOffset.tm_hour = (timeOffsetInMinutes % (60 * 24)) / 60;
            
    if ( (timeOffset.tm_hour < 0) || (timeOffset.tm_min < 0) ){
      serializedValue += "-";
      timeOffset.tm_hour *= -1;
      timeOffset.tm_min *= -1;
    }
    else{
      serializedValue += "+";
    }
    char * offSetString = new char[6];
    sprintf(offSetString, "%02i:%02i", timeOffset.tm_hour, timeOffset.tm_min);
    serializedValue += offSetString;
    delete [] offSetString;
  }

  BasicType::serialize(serializedValue.c_str());
  return m_buf;
}

xsd__dateTime* DateTime::deserialize(const char* buf){
  struct tm value;
  struct tm* pTm;
  char *Utc;
  char *Temp1;
  char *Temp2;
  char *Temp3;
  // Get local timezone offset
  time_t now = time(NULL);
  struct tm *temp = gmtime(&now);
  struct tm utcTime;
  memcpy(&utcTime, temp, sizeof(struct tm));
  temp = localtime(&now);
  struct tm localTime;
  memcpy(&localTime, temp, sizeof(struct tm));
  long utcTimeInSeconds = (utcTime.tm_year * 60 * 60 * 24 * 365)
      + (utcTime.tm_yday * 60 * 60 * 24)
      + (utcTime.tm_hour * 60 * 60)
      + (utcTime.tm_min * 60);
  long localTimeInSeconds = (localTime.tm_year * 60 * 60 * 24 * 365)
      + (localTime.tm_yday * 60 * 60 * 24)
      + (localTime.tm_hour * 60 * 60)
      + (localTime.tm_min * 60);
  time_t d = utcTimeInSeconds - localTimeInSeconds;
	
  //XSD_DATETIME format is: CCYY(-)MM(-)DDThh:mm:ss.ss...Z      CCYY(-)MM(-)DDThh:mm:ss.ss...+/-<UTC TIME DIFFERENCE>
    
  sscanf(buf, "%d-%d-%dT%d:%d:%d", &value.tm_year,&value.tm_mon, &value.tm_mday, &value.tm_hour, &value.tm_min, &value.tm_sec);
  value.tm_year -= 1900;
  value.tm_mon--;
  value.tm_isdst = -1;
        
  Temp2 = const_cast<char*>(strpbrk(buf, "T"));
  Temp3 = strrchr (Temp2, ':');
  Temp3[0] = '\0';
  unsigned long len = strlen (Temp2);
  Temp3[0] = ':';
  
  Temp1 = const_cast<char*>(strpbrk (buf, "Z"));
  if(Temp1!=NULL){
    //if the timezone is represented adding 'Z' at the end
    time_t tempmktime = mktime (&value); // convert tm object to seconds
    pTm = localtime (&tempmktime); // construct tm object from seconds
    memcpy (&value, pTm, sizeof (tm));
    time_t t = mktime (&value);
    t = labs (t - d);
    pTm = localtime (&t);
    }
  else if (len > (sizeof (char) * 6)){
    //if the timezone is represented using +/-hh:mm format
    Utc = strpbrk (Temp2, "+");
    if (Utc == NULL){
      Utc = strpbrk (Temp2, "-");
    }
    time_t timeInSecs = mktime (&value);
            
    int hours = 0;
    int minutes = 0;
    sscanf (Utc + 1, "%d:%d", &hours, &minutes);
    int secs = hours * 60 * 60 + minutes * 60;
    if ((Temp1 = strpbrk ((Utc), "+")) != NULL){
      timeInSecs -= secs;
    }
    else{
      timeInSecs += secs;
    }
    pTm = localtime (&timeInSecs);
    memcpy (&value, pTm, sizeof (tm));
    time_t t = mktime (&value);
    t = labs (t - d);
    pTm = localtime (&t);
    }
    else{
    //if the zone is not represented in the date. it is assumed that the sent time is localtime */
      time_t timeInSecs = mktime (&value);
      pTm = localtime (&timeInSecs);
    }    
    
    xsd__dateTime * ret = new xsd__dateTime;
    memcpy (ret, pTm, sizeof (tm));
    return ret;
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

};
