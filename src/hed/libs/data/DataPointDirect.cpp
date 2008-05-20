#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/StringConv.h>

#include "DataPointDirect.h"

namespace Arc {

  DataPointDirect::DataPointDirect(const URL& url)
    : DataPoint(url),
      buffer(NULL),
      bufsize(-1),
      bufnum(1),
      cache(true),
      local(false),
      readonly(true),
      linkable(false),
      is_secure(false),
      force_secure(true),
      force_passive(false),
      additional_checks(true),
      allow_out_of_order(false),
      range_start(0),
      range_end(0) {
    bufnum = stringtoi(url.Option("threads"));
    if (bufnum < 1)
      bufnum = 1;
    if (bufnum > MAX_PARALLEL_STREAMS)
      bufnum = MAX_PARALLEL_STREAMS;

    bufsize = stringtoull(url.Option("blocksize"));
    if (bufsize > MAX_BLOCK_SIZE)
      bufsize = MAX_BLOCK_SIZE;

    cache = (url.Option("cache", "yes") == "yes");
    readonly = (url.Option("readonly", "yes") == "yes");
  }

  DataPointDirect::~DataPointDirect() {}

  bool DataPointDirect::IsIndex() const {
    return false;
  }

  unsigned long long int DataPointDirect::BufSize() const {
    return bufsize;
  }

  int DataPointDirect::BufNum() const {
    return bufnum;
  }

  bool DataPointDirect::Cache() const {
    return cache;
  }

  bool DataPointDirect::Local() const {
    return local;
  }

  bool DataPointDirect::ReadOnly() const {
    return readonly;
  }

  void DataPointDirect::ReadOutOfOrder(bool val) {
    allow_out_of_order = val;
  }

  bool DataPointDirect::WriteOutOfOrder() {
    return false;
  }

  void DataPointDirect::SetAdditionalChecks(bool val) {
    additional_checks = val;
  }

  bool DataPointDirect::GetAdditionalChecks() const {
    return additional_checks;
  }

  void DataPointDirect::SetSecure(bool val) {
    force_secure = val;
  }

  bool DataPointDirect::GetSecure() const {
    return is_secure;
  }

  void DataPointDirect::Passive(bool val) {
    force_passive = val;
  }

  void DataPointDirect::Range(unsigned long long int start,
			      unsigned long long int end) {
    range_start = start;
    range_end = end;
  }

  DataStatus DataPointDirect::Resolve(bool) {
    return DataStatus::NotSupportedForDirectDataPointsError;
  }

  DataStatus DataPointDirect::PreRegister(bool, bool) {
    return DataStatus::NotSupportedForDirectDataPointsError;
  }

  DataStatus DataPointDirect::PostRegister(bool) {
    return DataStatus::NotSupportedForDirectDataPointsError;
  }

  DataStatus DataPointDirect::PreUnregister(bool) {
    return DataStatus::NotSupportedForDirectDataPointsError;
  }

  DataStatus DataPointDirect::Unregister(bool) {
    return DataStatus::NotSupportedForDirectDataPointsError;
  }

  bool DataPointDirect::AcceptsMeta() {
    return false;
  }

  bool DataPointDirect::ProvidesMeta() {
    return false;
  }

  bool DataPointDirect::Registered() const {
    return false;
  }

  const URL& DataPointDirect::CurrentLocation() const {
    return url;
  }

  const std::string& DataPointDirect::CurrentLocationMetadata() const {
    static const std::string empty;
    return empty;
  }

  bool DataPointDirect::NextLocation() {
    if (triesleft > 0)
      --triesleft;
    return (triesleft > 0);
  }

  bool DataPointDirect::LocationValid() const {
    return (triesleft > 0);
  }

  bool DataPointDirect::HaveLocations() const {
    return true;
  }

  DataStatus DataPointDirect::AddLocation(const URL&, const std::string&) {
    return DataStatus::NotSupportedForDirectDataPointsError;
  }

  DataStatus DataPointDirect::RemoveLocation() {
    return DataStatus::NotSupportedForDirectDataPointsError;
  }

  DataStatus DataPointDirect::RemoveLocations(const DataPoint&) {
    return DataStatus::NotSupportedForDirectDataPointsError;
  }

} // namespace Arc
