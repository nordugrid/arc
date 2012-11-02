// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>

#include <arc/client/ExecutionTarget.h>

#include "BenchmarkBrokerPlugin.h"

namespace Arc {
  bool BenchmarkBrokerPlugin::operator()(const ExecutionTarget& lhs, const ExecutionTarget& rhs) const {
    std::map<std::string, double>::const_iterator itLHS = lhs.Benchmarks->find(benchmark);
    std::map<std::string, double>::const_iterator itRHS = rhs.Benchmarks->find(benchmark);
    
    if (itLHS == lhs.Benchmarks->end()) { return false; }
    if (itRHS == rhs.Benchmarks->end()) { return true; }

    return itLHS->second > itRHS->second;
  }

  bool BenchmarkBrokerPlugin::match(const ExecutionTarget& et) const {
    if(!BrokerPlugin::match(et)) return false;
    return et.Benchmarks->find(benchmark) != et.Benchmarks->end();
  }
} // namespace Arc
