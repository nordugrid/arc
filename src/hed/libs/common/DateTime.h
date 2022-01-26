// -*- indent-tabs-mode: nil -*-

#ifndef ARCLIB_TIME
#define ARCLIB_TIME

#include <stdint.h>
#include <sigc++/slot.h>

#include <ctime>
#include <iostream>
#include <string>

namespace Arc {

  /** \addtogroup common
   *  @{ */

  /// An enumeration that contains the possible textual time formats.
  enum TimeFormat {
    MDSTime,       ///< YYYYMMDDHHMMSSZ
    ASCTime,       ///< Day Mon DD HH:MM:SS YYYY
    UserTime,      ///< YYYY-MM-DD HH:MM:SS
    ISOTime,       ///< YYYY-MM-DDTHH:MM:SS+HH:MM
    UTCTime,       ///< YYYY-MM-DDTHH:MM:SSZ
    RFC1123Time,   ///< Day, DD Mon YYYY HH:MM:SS GMT
    EpochTime,     ///< 1234567890
    UserExtTime,   ///< YYYY-MM-DD HH:MM:SS.mmmmmm (microseconds resolution)
    ElasticTime,   ///< YYYY-MM-DD HH:MM:SS.mmm (milliseconds resolution, suitable for Elasticsearch)
  };

  /// Base to use when constructing a new Period.
  enum PeriodBase {
    PeriodNanoseconds,  ///< Nanoseconds
    PeriodMicroseconds, ///< Microseconds
    PeriodMiliseconds,  ///< Milliseconds
    PeriodSeconds,      ///< Seconds
    PeriodMinutes,      ///< Minutes
    PeriodHours,        ///< Hours
    PeriodDays,         ///< Days
    PeriodWeeks         ///< Weeks
  };

  /// A Period represents a length of time.
  /** Period represents a length of time (eg 2 mins and 30.1 seconds), whereas
      Time represents a moment of time (eg midnight on 1st Jan 2000).
      \see Time
      \headerfile DateTime.h arc/DateTime.h */
  class Period {
  public:
    /// Default constructor. The period is set to 0 length.
    Period();

    /// Constructor that takes a time_t variable and stores it.
    Period(time_t);

    /// Constructor that takes seconds and nanoseconds and stores them.
    Period(time_t seconds, uint32_t nanoseconds);

    /// Constructor that tries to convert a string.
    Period(const std::string&, PeriodBase base = PeriodSeconds);

    /// Assignment operator from a time_t.
    Period& operator=(time_t);

    /// Assignment operator from a Period.
    Period& operator=(const Period&);

    /// Sets the period in seconds.
    void SetPeriod(time_t sec);
    /// Sets the period in seconds and nanoseconds.
    void SetPeriod(time_t sec, uint32_t nanosec);

    /// Gets the period in seconds.
    time_t GetPeriod() const;
    /// Gets the number of nanoseconds after the last whole second.
    uint32_t GetPeriodNanoseconds() const;

    /// For use with IString.
    const sigc::slot<const char*>* istr() const;

    /// Returns a string representation of the period.
    operator std::string() const;

    /// Comparing two Period objects.
    bool operator<(const Period&) const;

    /// Comparing two Period objects.
    bool operator>(const Period&) const;

    /// Comparing two Period objects.
    bool operator<=(const Period&) const;

    /// Comparing two Period objects.
    bool operator>=(const Period&) const;

    /// Comparing two Period objects.
    bool operator==(const Period&) const;

    /// Comparing two Period objects.
    bool operator!=(const Period&) const;
    
    Period& operator+=(const Period&);

  private:
    /// The duration of the period
    time_t seconds;
    uint32_t nanoseconds;

    /// Internal IString implementation
    const char* IStr() const;
    sigc::slot<const char*> slot;
    std::string is;
  };

  /// Prints a Period-object to the given ostream -- typically cout.
  std::ostream& operator<<(std::ostream&, const Period&);



  /// A class for storing and manipulating times.
  /** Time represents a moment of time (eg midnight on 1st Jan 2000), whereas
      Period represents a length of time (eg 2 mins and 30.1 seconds).
      \see Period
      \headerfile DateTime.h arc/DateTime.h */
  class Time {
  public:
    /// Default constructor. The time is put equal the current time.
    Time();

    /// Constructor that takes a time_t variable and stores it.
    Time(time_t);

    /// Constructor that takes a fine grained time variables and stores them.
    Time(time_t time, uint32_t nanosec);

    /// Constructor that tries to convert a string into a time_t.
    Time(const std::string&);

    /// Assignment operator from a time_t.
    Time& operator=(time_t);

    /// Assignment operator from a Time.
    Time& operator=(const Time&);

    /// Assignment operator from a char pointer.
    Time& operator=(const char*);
    
    /// Assignment operator from a string.
    Time& operator=(const std::string&);

    /// Sets the time.
    void SetTime(time_t);

    /// Sets the fine grained time.
    void SetTime(time_t time, uint32_t nanosec);

    /// Gets the time in seconds.
    time_t GetTime() const;
    /// Gets the nanoseconds fraction of the time.
    uint32_t GetTimeNanoseconds() const;

    /// Returns a string representation of the time, using the default format.
    operator std::string() const;

    /// Returns a string representation of the time, using the specified format.
    std::string str(const TimeFormat& = time_format) const;

    /// Sets the default format for time strings.
    static void SetFormat(const TimeFormat&);

    /// Gets the default format for time strings.
    static TimeFormat GetFormat();

    /// Comparing two Time objects.
    bool operator<(const Time&) const;

    /// Comparing two Time objects.
    bool operator>(const Time&) const;

    /// Comparing two Time objects.
    bool operator<=(const Time&) const;

    /// Comparing two Time objects.
    bool operator>=(const Time&) const;

    /// Comparing two Time objects.
    bool operator==(const Time&) const;

    /// Comparing two Time objects.
    bool operator!=(const Time&) const;

    /// Adding Time object with Period object.
    Time operator+(const Period&) const;

    /// Subtracting Period object from Time object.
    Time operator-(const Period&) const;

    /// Subtracting Time object from the other Time object.
    Period operator-(const Time&) const;

    /// Number of seconds in a year (365 days)
    static const int YEAR  = 31536000;
    /// Number of seconds in 30 days
    static const int MONTH = 2592000;
    /// Number of seconds in a week
    static const int WEEK  = 604800;
    /// Number of seconds in a day
    static const int DAY   = 86400;
    /// Number of seconds in an hour
    static const int HOUR  = 3600;
    /// Undefined time
    static const time_t UNDEFINED = (time_t)(-1);

  private:
    /// The time stored -- by default it is equal to the current time.
    time_t gtime;

    /// The nanosecond part of time stored -- by default it is 0.
    uint32_t gnano;

    /// The time-format stored. By default it is equal to UserTime
    static TimeFormat time_format;
  };

  /// Prints a Time-object to the given ostream -- typically cout.
  std::ostream& operator<<(std::ostream&, const Time&);

  /// Returns a time-stamp of the current time in some format.
  std::string TimeStamp(const TimeFormat& = Time::GetFormat());

  /// Returns a time-stamp of some specified time in some format.
  std::string TimeStamp(Time, const TimeFormat& = Time::GetFormat());

  /** @} */
} // namespace Arc

#endif // ARCLIB_TIME
