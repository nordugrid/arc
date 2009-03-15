// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/data/DataSpeed.h>

namespace Arc {

  bool DataSpeed::verbose(void) {
    return be_verbose;
  }

  void DataSpeed::verbose(bool val) {
    be_verbose = val;
  }

  void DataSpeed::verbose(const std::string& prefix) {
    be_verbose = true;
    verbose_prefix = prefix;
  }

  void DataSpeed::hold(bool disable) {
    disabled = disable;
  }

  bool DataSpeed::transfer(unsigned long long int n) {
    if (disabled) {
      last_time = time(NULL);
      return true;
    }
    time_t t = time(NULL);
    time_t dt = t - last_time;
    Nall += n;
    if (dt > T)
      N = (n * dt) / T;
    else
      N = (N * (T - dt)) / T + n;
    if ((t - first_time) >= T * 3) {
      /* make decision only after statistics settles */
      /* check for speed */
      if (N < (T * min_speed))
        if (first_speed_failure != 0) {
          if (t > (first_speed_failure + min_speed_time))
            min_speed_failed = true;
        }
        else
          first_speed_failure = t;
      else
        first_speed_failure = 0;
      /* check for avearge speed */
      if ((min_average_speed * (t - first_time)) > Nall)
        min_average_speed_failed = true;
      /* check for inactivity time */
      if (t > (last_activity_time + max_inactivity_time))
        max_inactivity_time_failed = true;
    }
    if (n > 0)
      last_activity_time = t;
    last_time = t;
    if (be_verbose)
      /* statistics to screen */
      if ((t - last_printed) >= 1) {
        print_statistics(stderr, t);
        last_printed = t;
      }
    return !(min_speed_failed || min_average_speed_failed ||
             max_inactivity_time_failed);
  }


  void DataSpeed::print_statistics(FILE *o, time_t t) {
    if (show_progress != NULL) {
      (*show_progress)(o, verbose_prefix.c_str(),
                       (unsigned int)(t - first_time), Nall, Nmax,
                       (t > first_time ?
                        (((double)N) / (((t - first_time) > T) ? T :
                                        (t - first_time))) : ((double)0)),
                       (t > first_time ?
                        (((double)Nall) / (t - first_time)) : ((double)0)));
      return;
    }
    fprintf(o,
            "%s%5u s: %10.1f kB  %8.1f kB/s  %8.1f kB/s    %c %c %c       \n",
            verbose_prefix.c_str(),
            (unsigned int)(t - first_time),
            ((double)Nall) / 1024,
            (t > first_time ?
             (((double)N) / (((t - first_time) > T) ? T :
                             (t - first_time)) / 1024) : ((double)0)),
            (t > first_time ?
             (((double)Nall) / (t - first_time) / 1024) : ((double)0)),
            (min_speed_failed ? '!' : '.'),
            (min_average_speed_failed ? '!' : '.'),
            (max_inactivity_time_failed ? '!' : '.'));
  }

  void DataSpeed::set_min_speed(unsigned long long int min_speed_,
                                time_t min_speed_time_) {
    min_speed = min_speed_;
    min_speed_time = min_speed_time_;
  }

  void DataSpeed::set_min_average_speed(unsigned long long int
                                        min_average_speed_) {
    min_average_speed = min_average_speed_;
  }

  void DataSpeed::set_max_inactivity_time(time_t max_inactivity_time_) {
    max_inactivity_time = max_inactivity_time_;
  }

  void DataSpeed::set_base(time_t base_) {
    N = (N * base_) / T; /* allows ro change T on the fly */
    T = base_;
  }

  void DataSpeed::set_progress_indicator(show_progress_t func) {
    show_progress = func;
  }

  void DataSpeed::set_max_data(unsigned long long int max) {
    Nmax = max;
  }

  DataSpeed::DataSpeed(unsigned long long int min_speed_,
                       time_t min_speed_time_,
                       unsigned long long int min_average_speed_,
                       time_t max_inactivity_time_, time_t base_) {
    min_speed = min_speed_;
    min_speed_time = min_speed_time_;
    min_average_speed = min_average_speed_;
    max_inactivity_time = max_inactivity_time_;
    T = base_;
    be_verbose = false;
    disabled = false;
    show_progress = NULL;
    Nmax = 0;
    reset();
  }

  DataSpeed::~DataSpeed(void) {
    /* statistics to screen */
    if (be_verbose)
      print_statistics(stderr, time(NULL));
  }

  DataSpeed::DataSpeed(time_t base_) {
    min_speed = 0;
    min_speed_time = 0;
    min_average_speed = 0;
    max_inactivity_time = 600; /* MUST have at least pure timeout */
    T = base_;
    be_verbose = false;
    disabled = false;
    show_progress = NULL;
    Nmax = 0;
    reset();
  }

  void DataSpeed::reset(void) {
    first_time = time(NULL);
    last_time = first_time;
    last_activity_time = first_time;
    last_printed = first_time;
    N = 0;
    Nall = 0;
    first_speed_failure = 0;
    min_speed_failed = false;
    min_average_speed_failed = false;
    max_inactivity_time_failed = false;
    return;
  }

} // namespace Arc
