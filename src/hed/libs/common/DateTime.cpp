// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <cstdio>
#include <iomanip>
#include <sstream>
#include <sys/time.h>

#include <arc/StringConv.h>
#include <arc/Logger.h>
#include <arc/Utils.h>
#include "DateTime.h"


#ifndef HAVE_TIMEGM
time_t timegm(struct tm *tm) {
  bool found;
  std::string tz = Arc::GetEnv("TZ", found);
  Arc::SetEnv("TZ", "UTC");
  tzset();
  tm->tm_isdst = -1;
  time_t ret = mktime(tm);
  if (found)
    Arc::SetEnv("TZ", tz);
  else
    Arc::UnsetEnv("TZ");
  tzset();
  return ret;
}
#endif


#ifndef HAVE_LOCALTIME_R
struct tm* localtime_r(const time_t *timep, struct tm *result) {
  struct tm *TM = localtime(timep);
  *result = *TM;
  return result;
}
#endif


#ifndef HAVE_GMTIME_R
struct tm* gmtime_r(const time_t *timep, struct tm *result) {
  struct tm *TM = gmtime(timep);
  *result = *TM;
  return result;
}
#endif


namespace Arc {

  static Logger dateTimeLogger(Logger::getRootLogger(), "DateTime");


  TimeFormat Time::time_format = UserTime;


  Time::Time()
    : gtime(0), gnano(0) {
    timeval tv;
    if(gettimeofday(&tv, NULL) != 0) {
      gtime = time(NULL);
    } else {
      gtime = tv.tv_sec;
      gnano = tv.tv_usec * 1000;
    }
  }


  Time::Time(time_t time)
    : gtime(time), gnano(0) {}


  Time::Time(time_t time, uint32_t nanosec)
    : gtime(time), gnano(nanosec) {}


  Time::Time(const std::string& timestring)
    : gtime(-1), gnano(0) {

    if (timestring.empty()) {
      dateTimeLogger.msg(ERROR, "Empty string");
      return;
    }

    if (isdigit(timestring[0])) {
      tm timestr = { 0 };
      std::string::size_type pos = 0;

      if (sscanf(timestring.substr(pos, 10).c_str(),
                 "%4d-%2d-%2d",
                 &timestr.tm_year,
                 &timestr.tm_mon,
                 &timestr.tm_mday) == 3)
        pos += 10;
      else if (sscanf(timestring.substr(pos, 8).c_str(),
                      "%4d%2d%2d",
                      &timestr.tm_year,
                      &timestr.tm_mon,
                      &timestr.tm_mday) == 3)
        pos += 8;
      else {
        dateTimeLogger.msg(ERROR, "Can not parse date: %s", timestring);
        return;
      }

      timestr.tm_year -= 1900;
      timestr.tm_mon--;

      if (timestring[pos] == 'T' || timestring[pos] == ' ')
        pos++;

      if (sscanf(timestring.substr(pos, 8).c_str(),
                 "%2d:%2d:%2d",
                 &timestr.tm_hour,
                 &timestr.tm_min,
                 &timestr.tm_sec) == 3)
        pos += 8;
      else if (sscanf(timestring.substr(pos, 6).c_str(),
                      "%2d%2d%2d",
                      &timestr.tm_hour,
                      &timestr.tm_min,
                      &timestr.tm_sec) == 3)
        pos += 6;
      else {
        dateTimeLogger.msg(ERROR, "Can not parse time: %s", timestring);
        return;
      }

      // skip fraction of second
      if (timestring[pos] == '.') {
        pos++;
        while (isdigit(timestring[pos]))
          pos++;
      }

      if (timestring[pos] == 'Z') {
        pos++;
        gtime = timegm(&timestr);
      }
      else if (timestring[pos] == '+' || timestring[pos] == '-') {
        bool tzplus = (timestring[pos] == '+');
        pos++;
        int tzh, tzm;
        if (sscanf(timestring.substr(pos, 5).c_str(),
                   "%2d:%2d",
                   &tzh,
                   &tzm) == 2)
          pos += 5;
        else if (sscanf(timestring.substr(pos, 4).c_str(),
                        "%2d%2d",
                        &tzh,
                        &tzm) == 2)
          pos += 4;
        else {
          dateTimeLogger.msg(ERROR, "Can not parse time zone offset: %s",
                             timestring);
          return;
        }

        gtime = timegm(&timestr);

        if (gtime != -1) {
          if (tzplus)
            gtime -= tzh * 3600 + tzm * 60;
          else
            gtime += tzh * 3600 + tzm * 60;
        }
      }
      else {
        timestr.tm_isdst = -1;
        gtime = mktime(&timestr);
      }

      if (timestring.size() != pos) {
        dateTimeLogger.msg(ERROR, "Illegal time format: %s", timestring);
        return;
      }
    }
    else if (timestring.length() == 24) {
      // C time
      tm timestr = { 0 };
      char day[4];
      char month[4];

      if (sscanf(timestring.c_str(),
                 "%3s %3s %2d %2d:%2d:%2d %4d",
                 day,
                 month,
                 &timestr.tm_mday,
                 &timestr.tm_hour,
                 &timestr.tm_min,
                 &timestr.tm_sec,
                 &timestr.tm_year) != 7) {
        dateTimeLogger.msg(ERROR, "Illegal time format: %s", timestring);
        return;
      }

      timestr.tm_year -= 1900;

      if (strncmp(month, "Jan", 3) == 0)
        timestr.tm_mon = 0;
      else if (strncmp(month, "Feb", 3) == 0)
        timestr.tm_mon = 1;
      else if (strncmp(month, "Mar", 3) == 0)
        timestr.tm_mon = 2;
      else if (strncmp(month, "Apr", 3) == 0)
        timestr.tm_mon = 3;
      else if (strncmp(month, "May", 3) == 0)
        timestr.tm_mon = 4;
      else if (strncmp(month, "Jun", 3) == 0)
        timestr.tm_mon = 5;
      else if (strncmp(month, "Jul", 3) == 0)
        timestr.tm_mon = 6;
      else if (strncmp(month, "Aug", 3) == 0)
        timestr.tm_mon = 7;
      else if (strncmp(month, "Sep", 3) == 0)
        timestr.tm_mon = 8;
      else if (strncmp(month, "Oct", 3) == 0)
        timestr.tm_mon = 9;
      else if (strncmp(month, "Nov", 3) == 0)
        timestr.tm_mon = 10;
      else if (strncmp(month, "Dec", 3) == 0)
        timestr.tm_mon = 11;
      else {
        dateTimeLogger.msg(ERROR, "Can not parse month: %s", month);
        return;
      }

      timestr.tm_isdst = -1;
      gtime = mktime(&timestr);
    }
    else if (timestring.length() == 29) {
      // RFC 1123 time (used by HTTP protoccol)
      tm timestr = { 0 };
      char day[4];
      char month[4];

      if (sscanf(timestring.c_str(),
                 "%3s, %2d %3s %4d %2d:%2d:%2d GMT",
                 day,
                 &timestr.tm_mday,
                 month,
                 &timestr.tm_year,
                 &timestr.tm_hour,
                 &timestr.tm_min,
                 &timestr.tm_sec) != 7) {
        dateTimeLogger.msg(ERROR, "Illegal time format: %s", timestring);
        return;
      }

      timestr.tm_year -= 1900;

      if (strncmp(month, "Jan", 3) == 0)
        timestr.tm_mon = 0;
      else if (strncmp(month, "Feb", 3) == 0)
        timestr.tm_mon = 1;
      else if (strncmp(month, "Mar", 3) == 0)
        timestr.tm_mon = 2;
      else if (strncmp(month, "Apr", 3) == 0)
        timestr.tm_mon = 3;
      else if (strncmp(month, "May", 3) == 0)
        timestr.tm_mon = 4;
      else if (strncmp(month, "Jun", 3) == 0)
        timestr.tm_mon = 5;
      else if (strncmp(month, "Jul", 3) == 0)
        timestr.tm_mon = 6;
      else if (strncmp(month, "Aug", 3) == 0)
        timestr.tm_mon = 7;
      else if (strncmp(month, "Sep", 3) == 0)
        timestr.tm_mon = 8;
      else if (strncmp(month, "Oct", 3) == 0)
        timestr.tm_mon = 9;
      else if (strncmp(month, "Nov", 3) == 0)
        timestr.tm_mon = 10;
      else if (strncmp(month, "Dec", 3) == 0)
        timestr.tm_mon = 11;
      else {
        dateTimeLogger.msg(ERROR, "Can not parse month: %s", month);
        return;
      }

      gtime = timegm(&timestr);
    }

    if (gtime == -1)
      dateTimeLogger.msg(ERROR, "Illegal time format: %s", timestring);
  }


  void Time::SetTime(time_t time) {
    gtime = time;
    gnano = 0;
  }


  void Time::SetTime(time_t time, uint32_t nanosec) {
    gtime = time;
    gnano = nanosec;
  }


  time_t Time::GetTime() const {
    return gtime;
  }


  uint32_t Time::GetTimeNanoseconds() const {
    return gnano;
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

    const char *day[] = {
      "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
    };
    // C week starts on Sunday - just live with it...
    const char *month[] = {
      "Jan", "Feb", "Mar", "Apr", "May", "Jun",
      "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
    };

    switch (format) {

    case ASCTime:  // Day Mon DD HH:MM:SS YYYY
      {
        tm tmtime;
        localtime_r(&gtime, &tmtime);

        std::stringstream ss;

        ss << std::setfill('0');

        ss << day[tmtime.tm_wday] << ' '
           << month[tmtime.tm_mon] << ' '
           << std::setw(2) << tmtime.tm_mday << ' '
           << std::setw(2) << tmtime.tm_hour << ':'
           << std::setw(2) << tmtime.tm_min << ':'
           << std::setw(2) << tmtime.tm_sec << ' '
           << std::setw(4) << tmtime.tm_year + 1900;

        return ss.str();
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

    case UserExtTime:
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
           << std::setw(2) << tmtime.tm_sec << '.'
           << std::setw(6) << gnano/1000;

        return ss.str();
      }

    case ElasticTime:
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
           << std::setw(2) << tmtime.tm_sec << '.'
           << std::setw(3) << gnano/1000000;

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
           << std::setw(2) << abs(tzoffset) / Time::HOUR << ':'
           << std::setw(2) << (abs(tzoffset) % Time::HOUR) / 60;

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

    case RFC1123Time:
      {
        tm tmtime;
        gmtime_r(&gtime, &tmtime);

        std::stringstream ss;

        ss << std::setfill('0');

        ss << day[tmtime.tm_wday] << ", "
           << std::setw(2) << tmtime.tm_mday << ' '
           << month[tmtime.tm_mon] << ' '
           << std::setw(4) << tmtime.tm_year + 1900 << ' '
           << std::setw(2) << tmtime.tm_hour << ':'
           << std::setw(2) << tmtime.tm_min << ':'
           << std::setw(2) << tmtime.tm_sec << " GMT";

        return ss.str();
      }
    case EpochTime:
      {
        return tostring(gtime);
      } 
    }
    return "";
  }


  bool Time::operator<(const Time& othertime) const {
    if(gtime == othertime.gtime) return gnano < othertime.gnano;
    return (CompareTime(gtime,othertime.gtime) < 0);
  }


  bool Time::operator>(const Time& othertime) const {
    if(gtime == othertime.gtime) return gnano > othertime.gnano;
    return (CompareTime(gtime,othertime.gtime) > 0);
  }


  bool Time::operator<=(const Time& othertime) const {
    if(gtime == othertime.gtime) return gnano <= othertime.gnano;
    return (CompareTime(gtime,othertime.gtime) <= 0);
  }


  bool Time::operator>=(const Time& othertime) const {
    if(gtime == othertime.gtime) return gnano >= othertime.gnano;
    return (CompareTime(gtime,othertime.gtime) >= 0);
  }


  bool Time::operator==(const Time& othertime) const {
    return (gtime == othertime.gtime) && (gnano == othertime.gnano);
  }


  bool Time::operator!=(const Time& othertime) const {
    return (gtime != othertime.gtime) || (gnano != othertime.gnano);
  }

  Time Time::operator+(const Period& duration) const {
    time_t t;
    uint32_t n;
    t = gtime + duration.GetPeriod();
    n = gnano + duration.GetPeriodNanoseconds();
    t += n / 1000000000;
    n = n % 1000000000;
    return (Time(t,n));
  }

  Time Time::operator-(const Period& duration) const {
    time_t t;
    uint32_t n = 0;
    t = gtime - duration.GetPeriod();
    if(duration.GetPeriodNanoseconds() > gnano) {
      --t;
      n = 1000000000;
    }
    n += gnano - duration.GetPeriodNanoseconds();
    return (Time(t,n));
  }

  Period Time::operator-(const Time& other) const {
    time_t t;
    uint32_t n = 0;
    t = gtime - other.gtime;
    if(gnano < other.gnano) {
      --t;
      n = 1000000000;
    }
    n += gnano - other.gnano;
    return (Period(t,n));
  }

  Time& Time::operator=(time_t newtime) {
    gtime = newtime;
    gnano = 0;
    return *this;
  }

  Time& Time::operator=(const Time& newtime) {
    gtime = newtime.gtime;
    gnano = newtime.gnano;
    return *this;
  }

  Time& Time::operator=(const char* newtime) {
    return operator=(std::string(newtime));
  }

  Time& Time::operator=(const std::string& newtime) {
    return *this = Arc::Time(newtime);
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


  Period::Period()
    : seconds(0), nanoseconds(0) {}


  Period::Period(time_t sec)
    : seconds(sec), nanoseconds(0) {}


  Period::Period(time_t sec, uint32_t nanosec )
    : seconds(sec), nanoseconds(nanosec) {}


  Period::Period(const std::string& period, PeriodBase base)
    : seconds(0), nanoseconds(0) {

    if (period.empty()) {
      dateTimeLogger.msg(ERROR, "Empty string");
      return;
    }

    if (period[0] == 'P') {
      // ISO duration
      std::string::size_type pos = 1;
      bool min = false; // months or minutes?
      while (pos < period.size()) {
        if (period[pos] == 'T')
          min = true;
        else {
          std::string::size_type pos2 = pos;
          while (pos2 < period.size() && isdigit(period[pos2]))
            pos2++;
          if (pos2 == pos || pos2 == period.size()) {
            dateTimeLogger.msg(ERROR, "Invalid ISO duration format: %s",
                               period);
            seconds = 0;
            return;
          }
          int num = stringtoi(period.substr(pos, pos2 - pos));
          pos = pos2;
          switch (period[pos]) {
          case 'Y':
            seconds += num * Time::YEAR;
            break;

          case 'W':
            seconds += num * Time::WEEK;
            min = true;
            break;

          case 'D':
            seconds += num * Time::DAY;
            min = true;
            break;

          case 'H':
            seconds += num * Time::HOUR;
            min = true;
            break;

          case 'M':
            if (min)
              seconds += num * 60;
            else {
              seconds += num * Time::MONTH;
              min = true;
            }
            break;

          case 'S':
            seconds += num;
            break;

          default:
            dateTimeLogger.msg(ERROR, "Invalid ISO duration format: %s",
                               period);
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

      for (std::string::size_type i = 0; i != period.length(); i++) {
        if (isdigit(period[i])) {
          if (pos == std::string::npos) {
            pos = i;
            len = 0;
          }
          len++;
        }
        else if (pos != std::string::npos) {
          switch (period[i]) {
          case 'w':
          case 'W':
            seconds += stringtoi(period.substr(pos, len)) * Time::WEEK;
            pos = std::string::npos;
            base = PeriodDays;
            break;

          case 'd':
          case 'D':
            seconds += stringtoi(period.substr(pos, len)) * Time::DAY;
            pos = std::string::npos;
            base = PeriodHours;
            break;

          case 'h':
          case 'H':
            seconds += stringtoi(period.substr(pos, len)) * Time::HOUR;
            pos = std::string::npos;
            base = PeriodMinutes;
            break;

          case 'm':
          case 'M':
            seconds += stringtoi(period.substr(pos, len)) * 60;
            pos = std::string::npos;
            base = PeriodSeconds;
            break;

          case 's':
          case 'S':
            seconds += stringtoi(period.substr(pos, len));
            pos = std::string::npos;
            base = PeriodMiliseconds;
            break;

          case ' ':
            break;

          default:
            dateTimeLogger.msg(ERROR, "Invalid period string: %s", period);
            seconds = 0;
            return;
            break;
          }
        }
      }

      if (pos != std::string::npos) {
        uint64_t n;
        uint32_t nn = 0;
        if(stringto(period.substr(pos, len), n)) {
          switch (base) {
          case PeriodNanoseconds:
            nn = n % 1000000000;
            n /= 1000000000;
            break;

          case PeriodMicroseconds:
            nn = (n % 1000000) * 1000;
            n /= 1000000;
            break;

          case PeriodMiliseconds:
            nn = (n % 1000) * 1000000;
            n /= 1000;
            break;

          case PeriodSeconds:
            break;

          case PeriodMinutes:
            n *= 60;
            break;

          case PeriodHours:
            n *= Time::HOUR;
            break;

          case PeriodDays:
            n *= Time::DAY;
            break;

          case PeriodWeeks:
            n *= Time::WEEK;
            break;
          }
          seconds += n;
          nanoseconds += nn;
        }
      }
    }
  }


  Period& Period::operator=(time_t length) {
    seconds = length;
    nanoseconds = 0;
    return *this;
  }

  Period& Period::operator=(const Period& newperiod) {
    seconds = newperiod.seconds;
    nanoseconds = newperiod.nanoseconds;
    return *this;
  }

  void Period::SetPeriod(time_t sec) {
    seconds = sec;
    nanoseconds = 0;
  }

  void Period::SetPeriod(time_t sec, uint32_t nanosec) {
    seconds = sec;
    nanoseconds = nanosec;
  }


  time_t Period::GetPeriod() const {
    return seconds;
  }

  uint32_t Period::GetPeriodNanoseconds() const {
    return nanoseconds;
  }

  const sigc::slot<const char*>* Period::istr() const {
    const_cast<Period*>(this)->slot = sigc::mem_fun(this, &Arc::Period::IStr);
    return &slot;
  }

  const char* Period::IStr() const {

    time_t remain = seconds;

    std::stringstream ss;

    /*
     Size of year, month and even day is variable.
     To avoid ambiguity let's keep only time parts.
 
    if (remain >= Time::YEAR) {
      ss << remain / Time::YEAR << " "
         << FindNTrans("year", "years", remain / Time::YEAR);
      remain %= Time::YEAR;
    }
    if (remain >= Time::MONTH) {
      if (remain != seconds) ss << " ";
      ss << remain / Time::MONTH << " "
         << FindNTrans("month", "months", remain / Time::MONTH);
      remain %= Time::MONTH;
    }
    if (remain >= Time::DAY) {
      if (remain != seconds) ss << " ";
      ss << remain / Time::DAY << " "
         << FindNTrans("day", "days", remain / Time::DAY);
      remain %= Time::DAY;
    }
    */
    if (remain >= Time::HOUR) {
      if (remain != seconds) ss << " ";
      ss << remain / Time::HOUR << " "
         << FindNTrans("hour", "hours", remain / Time::HOUR);
      remain %= Time::HOUR;
    }
    if (remain >= 60) {
      if (remain != seconds) ss << " ";
      ss << remain / 60 << " "
         << FindNTrans("minute", "minutes", remain / 60);
      remain %= 60;
    }
    if ((remain >= 1) || (seconds == 0)) {
      if (remain != seconds) ss << " ";
      ss << remain << " "
         << FindNTrans("second", "seconds", remain);
    }

    const_cast<Period*>(this)->is = ss.str();
    return is.c_str();
  }

  Period::operator std::string() const {
    time_t remain = seconds;

    std::stringstream ss;

    ss << 'P';
    /*
     Size of year, month and even day is variable.
     To avoid ambiguity let's keep only time parts.
 
    if (remain >= Time::YEAR) {
      ss << remain / Time::YEAR << 'Y';
      remain %= Time::YEAR;
    }
    if (remain >= Time::MONTH) {
      ss << remain / Time::MONTH << 'M';
      remain %= Time::MONTH;
    }
    if (remain >= Time::DAY) {
      ss << remain / Time::DAY << 'D';
      remain %= Time::DAY;
    }
    */
    if (remain) {
      ss << 'T';
      if (remain >= Time::HOUR) {
        ss << remain / Time::HOUR << 'H';
        remain %= Time::HOUR;
      }
      if (remain >= 60) {
        ss << remain / 60 << 'M';
        remain %= 60;
      }
      if ((remain >= 1) || (seconds == 0))
        ss << remain << 'S';
    } else {
      ss << "T0S";
    }

    return ss.str();
  }


  bool Period::operator<(const Period& othertime) const {
    if(seconds != othertime.seconds) return seconds < othertime.seconds;
    return nanoseconds < othertime.nanoseconds;
  }


  bool Period::operator>(const Period& othertime) const {
    if(seconds != othertime.seconds) return seconds > othertime.seconds;
    return nanoseconds > othertime.nanoseconds;
  }


  bool Period::operator<=(const Period& othertime) const {
    if(seconds != othertime.seconds) return seconds <= othertime.seconds;
    return nanoseconds <= othertime.nanoseconds;
  }


  bool Period::operator>=(const Period& othertime) const {
    if(seconds != othertime.seconds) return seconds >= othertime.seconds;
    return nanoseconds >= othertime.nanoseconds;
  }


  bool Period::operator==(const Period& othertime) const {
    return (seconds == othertime.seconds) && (nanoseconds == othertime.nanoseconds);
  }


  bool Period::operator!=(const Period& othertime) const {
    return (seconds != othertime.seconds) || (nanoseconds != othertime.nanoseconds);
  }

  Period& Period::operator+=(const Period&otherPeriod) {
    nanoseconds += otherPeriod.nanoseconds;
    seconds += otherPeriod.seconds + nanoseconds/1000000000;
    nanoseconds = nanoseconds%1000000000;
    
    return *this;
  }


  std::ostream& operator<<(std::ostream& out, const Period& period) {
    return (out << (std::string)period);
  }

} // namespace Arc
