// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_DATAEXTERNALHELPER_H__
#define __ARC_DATAEXTERNALHELPER_H__

#include <istream>
#include <ostream>

#include <arc/URL.h>
#include <arc/data/DataPoint.h>

namespace Arc {
  class DataExternalHelper {
  private:
    DataPoint* plugin;
    PluginsFactory* plugins;
    std::istream& instream;
    std::ostream& outstream;
    unsigned int threads;
    unsigned long long int bufsize; // Size of each buffer
    unsigned long long int range_start;
    unsigned long long int range_end;
    bool force_secure;
    bool force_passive;
  public:
    DataExternalHelper(char const * modulepath, char const * modulename, const URL& url, const UserConfig& usercfg, std::istream& instream, std::ostream& outstream);
    virtual ~DataExternalHelper();
    operator bool(void) const { return (plugin != NULL); };
    bool operator!(void) const { return (plugin == NULL); };
    void SetBufferSize(unsigned long long int size) { bufsize = size; };
    void SetRange(unsigned long long int start, unsigned long long int end) { range_start = start; range_end = end; };
   // void SetAllowOutOfOrder(bool out_of_order) { allow_out_of_order = out_of_order; };
    void SetSecure(bool secure) { force_secure = secure; };
    void SetPassive(bool passive) { force_passive = passive; };
    DataStatus Rename(const URL& newurl);
    DataStatus List(DataPoint::DataPointInfoType verb = DataPoint::INFO_TYPE_ALL);
    DataStatus Stat(DataPoint::DataPointInfoType verb = DataPoint::INFO_TYPE_ALL);
    DataStatus Check();
    DataStatus Remove();
    DataStatus CreateDirectory(bool with_parents=false);
    DataStatus Read();
    DataStatus Write();

    static Logger logger;
  };
}

#endif // __ARC_DATAEXTERNALHELPER_H__

