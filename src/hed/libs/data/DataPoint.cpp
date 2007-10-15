#ifdef HAVE_CONFIG_H
#include <config.h>
#endif 

#include <list>

#include <arc/Logger.h>

#include "DataPoint.h"

namespace Arc {

  Logger DataPoint::logger(Logger::rootLogger, "DataPoint");

  DataPoint::DataPoint(const URL& url) : url(url),
                                         size(-1),
                                         created(-1),
                                         valid(-1),
                                         tries_left(5) {}

  const URL& DataPoint::base_url() const {
    return url;
  }

  int DataPoint::GetTries() const {
    return tries_left;
  }

  void DataPoint::SetTries(const int n) {
    tries_left = std::max(0, n);
  }

} // namespace Arc
