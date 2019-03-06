// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_DATAPOINTSRM_H__
#define __ARC_DATAPOINTSRM_H__

#include <list>

#include <arc/Thread.h>
#include <arc/data/DataPointDirect.h>
#include <arc/data/DataHandle.h>

#include "srmclient/SRMClient.h"

namespace ArcDMCSRM {

  using namespace Arc;

  /**
   * The Storage Resource Manager (SRM) protocol allows access to data
   * distributed across physical storage through a unified namespace
   * and management interface. PrepareReading() or PrepareWriting() must
   * be used before reading or writing a physical file.
   *
   * This class is a loadable module and cannot be used directly. The DataHandle
   * class loads modules at runtime and should be used instead of this.
   */
  class DataPointSRM
    : public DataPointDirect {
  public:
    DataPointSRM(const URL& url, const UserConfig& usercfg, PluginArgument* arg);
    virtual ~DataPointSRM();
    static Plugin* Instance(PluginArgument *arg);
    virtual DataStatus PrepareReading(unsigned int timeout,
                                      unsigned int& wait_time);
    virtual DataStatus PrepareWriting(unsigned int timeout,
                                      unsigned int& wait_time);
    virtual DataStatus StartReading(DataBuffer& buffer);
    virtual DataStatus StartWriting(DataBuffer& buffer,
                                    DataCallback *space_cb = NULL);
    virtual DataStatus StopWriting();
    virtual DataStatus StopReading();
    virtual DataStatus FinishReading(bool error);
    virtual DataStatus FinishWriting(bool error);
    virtual DataStatus Check(bool check_meta);
    virtual DataStatus Remove();
    virtual DataStatus Stat(FileInfo& file, DataPointInfoType verb = INFO_TYPE_ALL);
    virtual DataStatus Stat(std::list<FileInfo>& files,
                            const std::list<DataPoint*>& urls,
                            DataPointInfoType verb = INFO_TYPE_ALL);
    virtual DataStatus List(std::list<FileInfo>& files, DataPointInfoType verb = INFO_TYPE_ALL);
    virtual DataStatus CreateDirectory(bool with_parents=false);
    virtual DataStatus Rename(const URL& newurl);
    virtual const std::string DefaultCheckSum() const;
    virtual bool ProvidesMeta() const;
    virtual bool IsStageable() const;
    virtual std::vector<URL> TransferLocations() const;
    virtual void ClearTransferLocations();
  private:
    SRMClientRequest *srm_request; /* holds SRM request ID between Prepare* and Finish* */
    static Logger logger;
    std::vector<URL> turls; /* TURLs returned from prepare methods */
    URL r_url; /* URL used for redirected operations in Start/Stop Reading/Writing */
    DataHandle *r_handle;  /* handle used for redirected operations in Start/Stop Reading/Writing */
    bool reading;
    bool writing;
    DataStatus ListFiles(std::list<FileInfo>& files, DataPointInfoType verb, int recursion);
    /** Check protocols given in list can be used, and if not remove them */
    void CheckProtocols(std::list<std::string>& transport_protocols);
    /** Select transfer protocols from URL option or hard-coded list */
    void ChooseTransferProtocols(std::list<std::string>& transport_protocols);
    /// Convert URL to SRM style, also URI-encoding path if necessary
    std::string CanonicSRMURL(const URL& srm_url);
    /// Convert SRM metadata into FileInfo object, inserting a new object into the list
    void FillFileInfo(std::list<FileInfo>& files, const struct SRMFileMetaData& srm_metadata);
  };

} // namespace ArcDMCSRM

#endif // __ARC_DATAPOINTSRM_H__
