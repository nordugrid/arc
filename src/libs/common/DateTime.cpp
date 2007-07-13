#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <cstdio>
#include <iomanip>
#include <sstream>

#include "DateTime.h"
#include "StringConv.h"
#include "Logger.h"


#ifndef HAVE_TIMEGM
time_t timegm (struct tm *tm) {
  char *tz = getenv("TZ");
  setenv("TZ", "", 1);
  tzset();
  time_t ret = mktime(tm);
  if(tz)
    setenv("TZ", tz, 1);
  else
    unsetenv("TZ");
  tzset();
  return ret;
}
#endif


namespace Arc {

  static Logger dateTimeLogger(Logger::rootLogger, "DateTime");


  TimeFormat Time::time_format = UserTime;


  Time::Time() {
    gtime = time(NULL);
  }


  Time::Time(const time_t& time) : gtime(time) {}


  Time::Time(const std::string& timestring) : gtime(-1) {

    if(isdigit(timestring[0])) {
      tm timestr;
      std::string::size_type pos = 0;

      if(sscanf(timestring.substr(pos, 10).c_str(),
		"%4d-%2d-%2d",
		&timestr.tm_year,
		&timestr.tm_mon,
		&timestr.tm_mday) == 3)
	pos += 10;
      else if(sscanf(timestring.substr(pos, 8).c_str(),
		     "%4d%2d%2d",
		     &timestr.tm_year,
		     &timestr.tm_mon,
		     &timestr.tm_mday) == 3)
	pos += 8;
      else {
	dateTimeLogger.msg(ERROR, "Can not parse date: %s",
			   timestring.c_str());
	return;
      }

      timestr.tm_year -= 1900;
      timestr.tm_mon--;

      if(timestring[pos] == 'T' || timestring[pos] == ' ') pos++;

      if(sscanf(timestring.substr(pos, 8).c_str(),
		"%2d:%2d:%2d",
		&timestr.tm_hour,
		&timestr.tm_min,
		&timestr.tm_sec) == 3)
	pos += 8;
      else if(sscanf(timestring.substr(pos, 6).c_str(),
		     "%2d%2d%2d",
		     &timestr.tm_hour,
		     &timestr.tm_min,
		     &timestr.tm_sec) == 3)
	pos += 6;
      else {
	dateTimeLogger.msg(ERROR, "Can not parse time: %s",
			   timestring.c_str());
	return;
      }

      // skip fraction of second
      if(timestring[pos] == '.') {
	pos++;
	while(isdigit(timestring[pos])) pos++;
      }

      if(timestring[pos] == 'Z') {
	pos++;
	gtime = timegm(&timestr);
      }

      else if(timestring[pos] == '+' || timestring[pos] == '-') {
	bool tzplus = (timestring[pos] == '+');
	pos++;
	int tzh, tzm;
	if(sscanf(timestring.substr(pos, 5).c_str(),
		  "%2d:%2d",
		  &tzh,
		  &tzm) == 2)
	  pos += 5;
	else if(sscanf(timestring.substr(pos, 4).c_str(),
		       "%2d%2d",
		       &tzh,
		       &tzm) == 2)
	  pos += 4;
	else {
	  dateTimeLogger.msg(ERROR, "Can not parse time zone offset: %s",
			     timestring.c_str());
	  return;
	}

	gtime = timegm(&timestr);

	if(gtime != -1) {
	  if(tzplus)
	    gtime -= tzh * 3600 + tzm * 60;
	  else
	    gtime += tzh * 3600 + tzm * 60;
	}
      }

      else
	gtime = mktime(&timestr);

      if(timestring.size() != pos) {
	dateTimeLogger.msg(ERROR, "Illegal time format: %s",
			   timestring.c_str());
	return;
      }
    }

    else {
      tm timestr;
      char day[4];
      char month[4];

      if(sscanf(timestring.c_str(),
		"%3s %3s %2d %2d:%2d:%2d %4d",
		day,
		month,
		&timestr.tm_mday,
		&timestr.tm_hour,
		&timestr.tm_min,
		&timestr.tm_sec,
		&timestr.tm_year) != 7) {
	dateTimeLogger.msg(ERROR, "Illegal time format: %s",
			   timestring.c_str());
	return;
      }

      timestr.tm_year -= 1900;

      if(strncmp(month, "Jan", 3) == 0)
	timestr.tm_mon = 0;
      else if(strncmp(month, "Feb", 3) == 0)
	timestr.tm_mon = 1;
      else if(strncmp(month, "Mar", 3) == 0)
	timestr.tm_mon = 2;
      else if(strncmp(month, "Apr", 3) == 0)
	timestr.tm_mon = 3;
      else if(strncmp(month, "May", 3) == 0)
	timestr.tm_mon = 4;
      else if(strncmp(month, "Jun", 3) == 0)
	timestr.tm_mon = 5;
      else if(strncmp(month, "Jul", 3) == 0)
	timestr.tm_mon = 6;
      else if(strncmp(month, "Aug", 3) == 0)
	timestr.tm_mon = 7;
      else if(strncmp(month, "Sep", 3) == 0)
	timestr.tm_mon = 8;
      else if(strncmp(month, "Oct", 3) == 0)
	timestr.tm_mon = 9;
      else if(strncmp(month, "Nov", 3) == 0)
	timestr.tm_mon = 10;
      else if(strncmp(month, "Dec", 3) == 0)
	timestr.tm_mon = 11;
      else {
	dateTimeLogger.msg(ERROR, "Can not parse month: %s", month);
	return;
      }

      gtime = mktime(&timestr);
    }

    if(gtime == -1)
      dateTimeLogger.msg(ERROR, "Illegal time format: %s",
			 timestring.c_str());
  }


  void Time::SetTime(const time_t& time) {
    gtime = time;
  }


  time_t Time::GetTime() const {
    return gtime;
  }


  void Time::SetFormat(const TimeFormat& format) {
    time_format = format;
  }


  TimeFormat Time::GetFormat() {
    return time_format;
  }


  Time::operator std::string() const {
    return str();
  }


  std::string Time::str(const TimeFormat& format) const {

    switch(format) {

    case ASCTime:
      {
	char timestr[26];
	ctime_r(&gtime, timestr);
	// chop the trailing \n
	return std::string(timestr, 24);
      }

    case UserTime:
      {
	tm tmtime;
	localtime_r(&gtime, &tmtime);

	std::stringstream ss;

	ss << std::setfill('0');

	ss << std::setw(4) << tmtime.tm_year + 1900 << '-'
	   << std::setw(2) << tmtime.tm_mon + 1 << '-'
	   << std::setw(2) << tmtime.tm_mday << ' '
	   << std::setw(2) << tmtime.tm_hour << ':'
	   << std::setw(2) << tmtime.tm_min << ':'
	   << std::setw(2) << tmtime.tm_sec;

	return ss.str();
      }

    case MDSTime:
      {
	tm tmtime;
	gmtime_r(&gtime, &tmtime);

	std::stringstream ss;

	ss << std::setfill('0');

	ss << std::setw(4) << tmtime.tm_year + 1900
	   << std::setw(2) << tmtime.tm_mon + 1
	   << std::setw(2) << tmtime.tm_mday
	   << std::setw(2) << tmtime.tm_hour
	   << std::setw(2) << tmtime.tm_min
	   << std::setw(2) << tmtime.tm_sec << 'Z';

	return ss.str();
      }

    case ISOTime:
      {
	tm tmtime;
	localtime_r(&gtime, &tmtime);
	time_t tzoffset = timegm(&tmtime) - gtime;

	std::stringstream ss;

	ss << std::setfill('0');

	ss << std::setw(4) << tmtime.tm_year + 1900 << '-'
	   << std::setw(2) << tmtime.tm_mon + 1 << '-'
	   << std::setw(2) << tmtime.tm_mday << 'T'
	   << std::setw(2) << tmtime.tm_hour << ':'
	   << std::setw(2) << tmtime.tm_min << ':'
	   << std::setw(2) << tmtime.tm_sec << (tzoffset < 0 ? '-' : '+')
	   << std::setw(2) << abs(tzoffset) / 3600 << ':'
	   << std::setw(2) << (abs(tzoffset) % 3600) / 60;

	return ss.str();
      }

    case UTCTime:
      {
	tm tmtime;
	gmtime_r(&gtime, &tmtime);

	std::stringstream ss;

	ss << std::setfill('0');

	ss << std::setw(4) << tmtime.tm_year + 1900 << '-'
	   << std::setw(2) << tmtime.tm_mon + 1 << '-'
	   << std::setw(2) << tmtime.tm_mday << 'T'
	   << std::setw(2) << tmtime.tm_hour << ':'
	   << std::setw(2) << tmtime.tm_min << ':'
	   << std::setw(2) << tmtime.tm_sec << 'Z';

	return ss.str();
      }

    }
    return "";
  }


  bool Time::operator<(const Time& othertime) const {
    return gtime < othertime.GetTime();
  }


  bool Time::operator>(const Time& othertime) const {
    return gtime > othertime.GetTime();
  }


  bool Time::operator<=(const Time& othertime) const {
    return gtime <= othertime.GetTime();
  }


  bool Time::operator>=(const Time& othertime) const {
    return gtime >= othertime.GetTime();
  }


  bool Time::operator==(const Time& othertime) const {
    return gtime == othertime.GetTime();
  }


  bool Time::operator!=(const Time& othertime) const {
    return gtime != othertime.GetTime();
  }


  Time& Time::operator=(const time_t& newtime) {
    gtime = newtime;
    return *this;
  }


  std::ostream& operator<<(std::ostream& out, const Time& time) {
    return (out << time.str());
  }


  std::string TimeStamp(const TimeFormat& format) {
    Time now;
    return now.str(format);
  }


  std::string TimeStamp(Time newtime, const TimeFormat& format) {
    return newtime.str(format);
  }


  Period::Period() : seconds(0) {}


  Period::Period(const time_t& length) : seconds(length) {}


  Period::Period(const std::string& period) : seconds(0) {

    if (period.empty()) {
      dateTimeLogger.msg(ERROR, "Empty period string");
      return;
    }

    if (period[0] == 'P') {
      // ISO duration
      std::string::size_type pos = 1;
      bool min = false; // months or minutes?
      while (pos < period.size()) {
	if (period[pos] == 'T') {
	  min = true;
	}
	else {
	  std::string::size_type pos2 = pos;
	  while (pos2 < period.size() && isdigit(period[pos2])) pos2++;
	  if (pos2 == pos || pos2 == period.size()) {
	    dateTimeLogger.msg(ERROR, "Invalid ISO duration format: %s",
			       period.c_str());
	    seconds = 0;
	    return;
	  }
	  int num = stringtoi(period.substr(pos, pos2 - pos));
	  pos = pos2;
	  switch (period[pos]) {
	  case 'Y':
	    seconds += num * (365 * 24 * 60 * 60);
	    break;
	  case 'W':
	    seconds += num * (7 * 24 * 60 * 60);
	    min = true;
	    break;
	  case 'D':
	    seconds += num * (24 * 60 * 60);
	    min = true;
	    break;
	  case 'H':
	    seconds += num * (60 * 60);
	    min = true;
	    break;
	  case 'M':
	    if (min)
	      seconds += num * 60;
	    else {
	      seconds += num * (30 * 24 * 60 * 60);
	      min = true;
	    }
	    break;
	  case 'S':
	    seconds += num;
	    break;
          default:
	    dateTimeLogger.msg(ERROR, "Invalid ISO duration format: %s",
			       period.c_str());
	    seconds = 0;
	    return;
	    break;
	  }
	}
	pos++;
      }
    }

    else {
      // "free" format 
      std::string::size_type pos = std::string::npos;
      int len = 0;

      for(std::string::size_type i = 0; i != period.length(); i++) {
	if(isdigit(period[i])) {
	  if(pos == std::string::npos) {
	    pos = i;
	    len = 0;
	  }
	  len++;
	}
	else if(pos != std::string::npos) {
	  switch(period[i]) {
          case 'w':
          case 'W':
	    seconds += stringtoi(period.substr(pos, len)) * 60 * 60 * 24 * 7;
	    pos = std::string::npos;
	    break;
          case 'd':
          case 'D':
	    seconds += stringtoi(period.substr(pos, len)) * 60 * 60 * 24;
	    pos = std::string::npos;
	    break;
          case 'h':
          case 'H':
	    seconds += stringtoi(period.substr(pos, len)) * 60 * 60;
	    pos = std::string::npos;
	    break;
          case 'm':
          case 'M':
	    seconds += stringtoi(period.substr(pos, len)) * 60;
	    pos = std::string::npos;
	    break;
          case 's':
          case 'S':
	    seconds += stringtoi(period.substr (pos, len));
	    pos = std::string::npos;
	    break;
          case ' ':
	    break;
          default:
	    dateTimeLogger.msg(ERROR, "Invalid period string: %s",
			       period.c_str());
	    seconds = 0;
	    return;
	    break;
	  }
	}
      }

      if(pos != std::string::npos)
	seconds += stringtoi(period.substr(pos, len));
    }
  }


  Period& Period::operator=(const time_t& length) {
    seconds = length;
    return *this;
  }


  void Period::SetPeriod(const time_t& length) {
    seconds = length;
  }


  time_t Period::GetPeriod() const {
    return seconds;
  }


  Period::operator std::string() const {
    time_t remain = seconds;

    std::stringstream ss;

    ss << 'P';
    if (remain >= 365 * 24 * 60 * 60) {
      ss << remain / (365 * 24 * 60 * 60) << 'Y';
      remain %= (365 * 24 * 60 * 60);
    }
    if (remain >= 30 * 24 * 60 * 60) {
      ss << remain / (30 * 24 * 60 * 60) << 'M';
      remain %= (30 * 24 * 60 * 60);
    }
    if (remain >= 24 * 60 * 60) {
      ss << remain / (24 * 60 * 60) << 'D';
      remain %= (24 * 60 * 60);
    }
    if (remain)
      ss << 'T';
    if (remain >= 60 * 60) {
      ss << remain / (60 * 60) << 'H';
      remain %= (60 * 60);
    }
    if (remain >= 60) {
      ss << remain / 60 << 'M';
      remain %= 60;
    }
    if (remain >= 1)
      ss << remain  << 'S';

    return ss.str();
  }


  std::ostream& operator<<(std::ostream& out, const Period& period) {
    return (out << (std::string) period);
  }

} // namespace Arc
