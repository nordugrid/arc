// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_DATAPOINTINDEX_H__
#define __ARC_DATAPOINTINDEX_H__

#include <list>
#include <string>

#include <arc/data/DataHandle.h>
#include <arc/data/DataPoint.h>

namespace Arc {

  /// DataPointIndex represents "index" data objects, e.g. catalogs.
  /**
   * This class should never be used directly, instead inherit from it to
   * provide a class for a specific indexing service.
   * \ingroup data
   * \headerfile DataPointIndex.h arc/data/DataPointIndex.h
   */
  class DataPointIndex
    : public DataPoint {
  public:
    virtual ~DataPointIndex();

    virtual const URL& CurrentLocation() const;
    virtual const std::string& CurrentLocationMetadata() const;
    virtual DataPoint* CurrentLocationHandle() const;
    virtual DataStatus CompareLocationMetadata() const;
    virtual bool NextLocation();
    virtual bool LocationValid() const;
    virtual bool HaveLocations() const;
    virtual bool LastLocation();
    virtual DataStatus RemoveLocation();
    virtual DataStatus RemoveLocations(const DataPoint& p);
    virtual DataStatus ClearLocations();
    virtual DataStatus AddLocation(const URL& url, const std::string& meta);
    virtual void SortLocations(const std::string& pattern,
                               const URLMap& url_map);

    virtual bool IsIndex() const;
    virtual bool IsStageable() const;
    virtual bool AcceptsMeta() const;
    virtual bool ProvidesMeta() const;
    virtual void SetMeta(const DataPoint& p);
    virtual void SetCheckSum(const std::string& val);
    virtual void SetSize(const unsigned long long int val);
    virtual bool Registered() const;
    virtual bool SupportsTransfer() const;
    virtual void SetTries(const int n);

    // the following are relayed to the current location
    virtual long long int BufSize() const;
    virtual int BufNum() const;
    virtual bool Local() const;
    virtual bool ReadOnly() const;
    virtual DataStatus PrepareReading(unsigned int timeout,
                                      unsigned int& wait_time);
    virtual DataStatus PrepareWriting(unsigned int timeout,
                                      unsigned int& wait_time);
    virtual DataStatus StartReading(DataBuffer& buffer);
    virtual DataStatus StartWriting(DataBuffer& buffer,
                                    DataCallback *space_cb = NULL);
    virtual DataStatus StopReading();
    virtual DataStatus StopWriting();
    virtual DataStatus FinishReading(bool error = false);
    virtual DataStatus FinishWriting(bool error = false);
    virtual std::vector<URL> TransferLocations() const;
    virtual void ClearTransferLocations();
    virtual DataStatus Transfer(const URL& otherendpoint, bool source,
                                TransferCallback callback = NULL);

    virtual DataStatus Check(bool check_meta);

    virtual DataStatus Remove();

    virtual void ReadOutOfOrder(bool v);
    virtual bool WriteOutOfOrder() const;

    virtual void SetAdditionalChecks(bool v);
    virtual bool GetAdditionalChecks() const;

    virtual void SetSecure(bool v);
    virtual bool GetSecure() const;

    virtual DataPointAccessLatency GetAccessLatency() const;

    virtual void Passive(bool v);

    virtual void Range(unsigned long long int start = 0,
                       unsigned long long int end = 0);

    virtual int AddCheckSumObject(CheckSum *cksum);

    virtual const CheckSum* GetCheckSumObject(int index) const;

  protected:
    bool resolved;
    bool registered;
    DataPointIndex(const URL& url, const UserConfig& usercfg, PluginArgument* parg);

  private:
    // Following members must be kept synchronised hence they are private
    /// List of locations at which file can be probably found.
    std::list<URLLocation> locations;
    std::list<URLLocation>::iterator location;
    DataHandle *h;
    void SetHandle();
  };

} // namespace Arc

#endif // __ARC_DATAPOINTINDEX_H__
