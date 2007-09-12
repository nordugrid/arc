#ifndef ARCLIB_TIME
#define ARCLIB_TIME

/*
#ifdef HAVE_CONFIG_H
#include "config.h" // to get HAVE_TIMEGM
#endif
*/

#include <ctime>
#include <iostream>
#include <string>

#ifndef HAVE_TIMEGM
time_t timegm (struct tm *tm);
#endif

namespace Arc {

  /// An enumeration that contains the possible textual timeformats.
  enum TimeFormat {
    MDSTime,       // YYYYMMDDHHMMSSZ
    ASCTime,       // Day Mon DD HH:MM:SS YYYY
    UserTime,      // YYYY-MM-DD HH:MM:SS
    ISOTime,       // YYYY-MM-DDTHH:MM:SS+HH:MM
    UTCTime,       // YYYY-MM-DDTHH:MM:SSZ
    RFC1123Time    // Day, DD Mon YYYY HH:MM:SS GMT
  };


  class Period {
   public:
    /** Default constructor. The period is set to 0 length. */
    Period();

    /** Constructor that takes a time_t variable and stores it. */
    Period(const time_t&);

    /** Constructor that tries to convert a string. */
    Period(const std::string&);

    /** Assignment operator from a time_t. */
    Period& operator=(const time_t&);

    /** Assignment operator from a Period. */
    Period& operator=(const Period&);

    /** sets the period */
    void SetPeriod(const time_t&);

    /** gets the period */
    time_t GetPeriod() const;

    /** Returns a string representation of the period. */
    operator std::string() const;

   private:
    /** The duration of the period */
    time_t seconds;
  };

  /** Prints a Period-object to the given ostream -- typically cout. */
  std::ostream& operator<<(std::ostream&, const Period&);



  /// A class for storing and manipulating times.
  class Time {
   public:
    /** Default constructor. The time is put equal the current time. */
    Time();

    /** Constructor that takes a time_t variable and stores it. */
    Time(const time_t&);

    /** Constructor that tries to convert a string into a time_t. */
    Time(const std::string&);

    /** Assignment operator from a time_t. */
    Time& operator=(const time_t&);

    /** Assignment operator from a Time. */
    Time& operator=(const Time&);

    /** sets the time */
    void SetTime(const time_t&);

    /** gets the time */
    time_t GetTime() const;

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

   private:
    /** The time stored -- by default it is equal to the current time. */
    time_t gtime;

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
