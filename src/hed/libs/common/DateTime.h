// -*- indent-tabs-mode: nil -*-

#ifndef ARCLIB_TIME
#define ARCLIB_TIME

#include <stdint.h>
#include <sigc++/slot.h>

#include <ctime>
#include <iostream>
#include <string>

namespace Arc {

  /// An enumeration that contains the possible textual timeformats.
  enum TimeFormat {
    MDSTime,       // YYYYMMDDHHMMSSZ
    ASCTime,       // Day Mon DD HH:MM:SS YYYY
    UserTime,      // YYYY-MM-DD HH:MM:SS
    ISOTime,       // YYYY-MM-DDTHH:MM:SS+HH:MM
    UTCTime,       // YYYY-MM-DDTHH:MM:SSZ
    RFC1123Time,   // Day, DD Mon YYYY HH:MM:SS GMT
    EpochTime      // 1234567890
  };

  enum PeriodBase {
    PeriodNanoseconds,
    PeriodMicroseconds,
    PeriodMiliseconds,
    PeriodSeconds,
    PeriodMinutes,
    PeriodHours,
    PeriodDays,
    PeriodWeeks
  };

  class Period {
  public:
    /** Default constructor. The period is set to 0 length. */
    Period();

    /** Constructor that takes a time_t variable and stores it. */
    Period(time_t);

    /** Constructor that takes seconds and nanoseconds and stores them. */
    Period(time_t seconds, uint32_t nanoseconds);

    /** Constructor that tries to convert a string. */
    Period(const std::string&, PeriodBase base = PeriodSeconds);

    /** Assignment operator from a time_t. */
    Period& operator=(time_t);

    /** Assignment operator from a Period. */
    Period& operator=(const Period&);

    /** sets the period */
    void SetPeriod(time_t);
    void SetPeriod(time_t sec, uint32_t nanosec);

    /** gets the period */
    time_t GetPeriod() const;
    time_t GetPeriodNanoseconds() const;

    /** For use with IString */
    const sigc::slot<const char*>* istr() const;

    /** Returns a string representation of the period. */
    operator std::string() const;

    /** Comparing two Period objects. */
    bool operator<(const Period&) const;

    /** Comparing two Period objects. */
    bool operator>(const Period&) const;

    /** Comparing two Period objects. */
    bool operator<=(const Period&) const;

    /** Comparing two Period objects. */
    bool operator>=(const Period&) const;

    /** Comparing two Period objects. */
    bool operator==(const Period&) const;

    /** Comparing two Period objects. */
    bool operator!=(const Period&) const;

  private:
    /** The duration of the period */
    time_t seconds;
    uint32_t nanoseconds;

    /** Internal IString implementation */
    const char* IStr() const;
    sigc::slot<const char*> slot;
    std::string is;
  };

  /** Prints a Period-object to the given ostream -- typically cout. */
  std::ostream& operator<<(std::ostream&, const Period&);



  /// A class for storing and manipulating times.
  class Time {
  public:
    /** Default constructor. The time is put equal the current time. */
    Time();

    /** Constructor that takes a time_t variable and stores it. */
    Time(time_t);

    /** Constructor that takes a fine grained time variables and stores them. */
    Time(time_t time, uint32_t nanosec);

    /** Constructor that tries to convert a string into a time_t. */
    Time(const std::string&);

    /** Assignment operator from a time_t. */
    Time& operator=(time_t);

    /** Assignment operator from a Time. */
    Time& operator=(const Time&);

    /** Assignment operator from a char pointer. */
    Time& operator=(const char*);
    
    /** Assignment operator from a string. */
    Time& operator=(const std::string&);

    /** sets the time */
    void SetTime(time_t);

    /** sets the fine grained time */
    void SetTime(time_t time, uint32_t nanosec);

    /** gets the time */
    time_t GetTime() const;
    time_t GetTimeNanosec() const;

    /** Returns a string representation of the time,
        using the default format. */
    operator std::string() const;

    /** Returns a string representation of the time,
        using the specified format. */
    std::string str(const TimeFormat& = time_format) const;

    /** Sets the default format for time strings. */
    static void SetFormat(const TimeFormat&);

    /** Gets the default format for time strings. */
    static TimeFormat GetFormat();

    /** Comparing two Time objects. */
    bool operator<(const Time&) const;

    /** Comparing two Time objects. */
    bool operator>(const Time&) const;

    /** Comparing two Time objects. */
    bool operator<=(const Time&) const;

    /** Comparing two Time objects. */
    bool operator>=(const Time&) const;

    /** Comparing two Time objects. */
    bool operator==(const Time&) const;

    /** Comparing two Time objects. */
    bool operator!=(const Time&) const;

    /** Adding Time object with Period object. */
    Time operator+(const Period&) const;

    /** Subtracting Period object from Time object. */
    Time operator-(const Period&) const;

    /** Subtracting Time object from the other Time object. */
    Period operator-(const Time&) const;

    static const int YEAR  = 31536000;
    static const int MONTH = 2592000;
    static const int WEEK  = 604800;
    static const int DAY   = 86400;
    static const int HOUR  = 3600;

  private:
    /** The time stored -- by default it is equal to the current time. */
    time_t gtime;

    /** The nanosecond part of time stored -- by default it is 0. */
    uint32_t gnano;

    /** The time-format stored. By default it is equal to UserTime */
    static TimeFormat time_format;
  };

  /** Prints a Time-object to the given ostream -- typically cout. */
  std::ostream& operator<<(std::ostream&, const Time&);

  /** Returns a time-stamp of the current time in some format. */
  std::string TimeStamp(const TimeFormat& = Time::GetFormat());

  /** Returns a time-stamp of some specified time in some format. */
  std::string TimeStamp(Time, const TimeFormat& = Time::GetFormat());

} // namespace Arc

#endif // ARCLIB_TIME
