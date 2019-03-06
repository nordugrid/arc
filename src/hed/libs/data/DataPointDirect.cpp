// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/StringConv.h>
#include <arc/data/DataPointDirect.h>
#include <arc/CheckSum.h>

namespace Arc {

  DataPointDirect::DataPointDirect(const URL& url, const UserConfig& usercfg, PluginArgument* parg)
    : DataPoint(url, usercfg, parg),
      buffer(NULL),
      bufsize(-1),
      bufnum(1),
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
    std::string optval;

    optval = url.Option("threads");
    if (!optval.empty())
      bufnum = stringtoi(optval);
    if (bufnum < 1)
      bufnum = 1;
    if (bufnum > MAX_PARALLEL_STREAMS)
      bufnum = MAX_PARALLEL_STREAMS;

    optval = url.Option("blocksize");
    if (!optval.empty())
      bufsize = stringtoull(optval);
    if (bufsize > MAX_BLOCK_SIZE)
      bufsize = MAX_BLOCK_SIZE;

    readonly = (url.Option("readonly", "yes") == "yes");
  }

  DataPointDirect::~DataPointDirect() {}

  bool DataPointDirect::IsIndex() const {
    return false;
  }

  bool DataPointDirect::IsStageable() const {
    return false;
  }

  long long int DataPointDirect::BufSize() const {
    return bufsize;
  }

  int DataPointDirect::BufNum() const {
    return bufnum;
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
    return DataStatus(DataStatus::NotSupportedForDirectDataPointsError, EOPNOTSUPP);
  }

  DataStatus DataPointDirect::Resolve(bool source, const std::list<DataPoint*>& urls) {
    return DataStatus(DataStatus::NotSupportedForDirectDataPointsError, EOPNOTSUPP);
  }

  DataStatus DataPointDirect::PreRegister(bool, bool) {
    return DataStatus(DataStatus::NotSupportedForDirectDataPointsError, EOPNOTSUPP);
  }

  DataStatus DataPointDirect::PostRegister(bool) {
    return DataStatus(DataStatus::NotSupportedForDirectDataPointsError, EOPNOTSUPP);
  }

  DataStatus DataPointDirect::PreUnregister(bool) {
    return DataStatus(DataStatus::NotSupportedForDirectDataPointsError, EOPNOTSUPP);
  }

  DataStatus DataPointDirect::Unregister(bool) {
    return DataStatus(DataStatus::NotSupportedForDirectDataPointsError, EOPNOTSUPP);
  }

  bool DataPointDirect::AcceptsMeta() const {
    return false;
  }

  bool DataPointDirect::ProvidesMeta() const {
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
  
  DataPoint* DataPointDirect::CurrentLocationHandle() const {
    return const_cast<DataPointDirect*> (this);
  }

  DataStatus DataPointDirect::CompareLocationMetadata() const {
    return DataStatus(DataStatus::NotSupportedForDirectDataPointsError, EOPNOTSUPP);
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

  bool DataPointDirect::LastLocation() {
    return (triesleft == 1 || triesleft == 0);
  }

  DataStatus DataPointDirect::AddLocation(const URL&, const std::string&) {
    return DataStatus(DataStatus::NotSupportedForDirectDataPointsError, EOPNOTSUPP);
  }

  DataStatus DataPointDirect::RemoveLocation() {
    return DataStatus(DataStatus::NotSupportedForDirectDataPointsError, EOPNOTSUPP);
  }

  DataStatus DataPointDirect::RemoveLocations(const DataPoint&) {
    return DataStatus(DataStatus::NotSupportedForDirectDataPointsError, EOPNOTSUPP);
  }

  DataStatus DataPointDirect::ClearLocations() {
    return DataStatus(DataStatus::NotSupportedForDirectDataPointsError, EOPNOTSUPP);
  }

  int DataPointDirect::AddCheckSumObject(CheckSum *cksum) {
    if(!cksum)
      return -1;
    cksum->start();
    checksums.push_back(cksum);
    return checksums.size()-1;
  }

  const CheckSum* DataPointDirect::GetCheckSumObject(int index) const {
    if(index < 0) return NULL;
    if(index >= checksums.size()) return NULL;
    for(std::list<CheckSum*>::const_iterator cksum = checksums.begin();
               cksum != checksums.end(); ++cksum) {
      if(!index) return *cksum;
    }
    return NULL;
  }

  DataStatus DataPointDirect::Stat(std::list<FileInfo>& files,
                                   const std::list<DataPoint*>& urls,
                                   DataPointInfoType verb) {
    return DataStatus(DataStatus::UnimplementedError, EOPNOTSUPP);
  }


} // namespace Arc
