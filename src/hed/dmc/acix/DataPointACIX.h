#ifndef __ARC_DATAPOINTACIX_H__
#define __ARC_DATAPOINTACIX_H__

#include <arc/Logger.h>
#include <arc/URL.h>
#include <arc/data/DataPointIndex.h>

namespace ArcDMCACIX {

  /**
   * ACIX is the ARC Cache Index. This is a service which maps file URLs to ARC
   * CEs where the file is cached. These CEs can be configured to expose the
   * cache content through the A-REX HTTP interface and thus cached files can
   * be downloaded from the CEs.
   *
   * ACIX is special in that it can be used in addition to the regular file
   * location. Since the ARC data library cannot handle two separate possible
   * sources for a file, in order to use the original source location and ACIX
   * location the original source should be added to the ACIX datapoint using
   * AddLocation() before calling any other methods. Resolve() resolves both
   * the ACIX locations and the original locations (if it is an index URL).
   * Then the locations of the DataPointACIX will be both the ACIX locations
   * and the (resolved) original location(s). Only the cache locations which
   * are accessible from the outside are considered.
   */
  class DataPointACIX
    : public Arc::DataPointIndex {
  public:
    DataPointACIX(const Arc::URL& url, const Arc::UserConfig& usercfg, Arc::PluginArgument* parg);
    ~DataPointACIX();
    static Plugin* Instance(Arc::PluginArgument *arg);
    virtual Arc::DataStatus Resolve(bool source);
    virtual Arc::DataStatus Resolve(bool source, const std::list<DataPoint*>& urls);
    virtual Arc::DataStatus Check(bool check_meta);
    virtual Arc::DataStatus PreRegister(bool replication, bool force = false);
    virtual Arc::DataStatus PostRegister(bool replication);
    virtual Arc::DataStatus PreUnregister(bool replication);
    virtual Arc::DataStatus Unregister(bool all);
    virtual Arc::DataStatus Stat(Arc::FileInfo& file, Arc::DataPoint::DataPointInfoType verb = INFO_TYPE_ALL);
    virtual Arc::DataStatus Stat(std::list<Arc::FileInfo>& files,
                                 const std::list<Arc::DataPoint*>& urls,
                                 Arc::DataPoint::DataPointInfoType verb = INFO_TYPE_ALL);
    virtual Arc::DataStatus List(std::list<Arc::FileInfo>& files, Arc::DataPoint::DataPointInfoType verb = INFO_TYPE_ALL);
    virtual Arc::DataStatus CreateDirectory(bool with_parents=false);
    virtual Arc::DataStatus Rename(const Arc::URL& newurl);
    virtual Arc::DataStatus AddLocation(const Arc::URL& url, const std::string& meta);
    virtual const Arc::URL& GetURL() const;
    virtual std::string str() const;
  protected:
    static Arc::Logger logger;
    Arc::URLLocation original_location;
    bool original_location_resolved;
  private:
    Arc::DataStatus queryACIX(std::string& content, const std::string& path) const;
    Arc::DataStatus parseLocations(const std::string& content, const std::list<DataPoint*>& urls) const;

  };

} // namespace ArcDMCACIX

#endif /* __ARC_DATAPOINTACIX_H__ */
