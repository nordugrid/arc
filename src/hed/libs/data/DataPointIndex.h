#ifndef __ARC_DATAPOINTINDEX_H__
#define __ARC_DATAPOINTINDEX_H__

#include <string>
#include <list>

#include <arc/data/DataPoint.h>
#include <arc/data/DataHandle.h>

namespace Arc {

  /// Complements DataPoint with attributes common for Indexing Service URLs
  /** It should never be used directly. Instead inherit from it to provide
      a class for specific a Indexing Service. */
  class DataPointIndex
    : public DataPoint {
  public:
    DataPointIndex(const URL& url);
    virtual ~DataPointIndex();

    virtual const URL& CurrentLocation() const;
    virtual const std::string& CurrentLocationMetadata() const;

    virtual bool NextLocation();
    virtual bool LocationValid() const;
    virtual bool HaveLocations() const;
    virtual DataStatus RemoveLocation();
    virtual DataStatus RemoveLocations(const DataPoint& p);
    virtual DataStatus AddLocation(const URL& url, const std::string& meta);

    virtual bool IsIndex() const;
    virtual bool AcceptsMeta();
    virtual bool ProvidesMeta();
    virtual bool Registered() const;

    virtual void SetTries(const int n);

    // the following are relayed to the current location
    virtual unsigned long long int BufSize() const;
    virtual int BufNum() const;
    virtual bool Cache() const;
    virtual bool Local() const;
    virtual bool ReadOnly() const;

    virtual DataStatus StartReading(DataBufferPar& buffer);
    virtual DataStatus StartWriting(DataBufferPar& buffer,
				    DataCallback *space_cb = NULL);
    virtual DataStatus StopReading();
    virtual DataStatus StopWriting();

    virtual DataStatus Check();

    virtual DataStatus Remove();

    virtual void ReadOutOfOrder(bool v);
    virtual bool WriteOutOfOrder();

    virtual void SetAdditionalChecks(bool v);
    virtual bool GetAdditionalChecks() const;

    virtual void SetSecure(bool v);
    virtual bool GetSecure() const;

    virtual void Passive(bool v);

    virtual void Range(unsigned long long int start = 0,
		       unsigned long long int end = 0);
  protected:
    /// List of locations at which file can be probably found.
    std::list<URLLocation> locations;
    std::list<URLLocation>::iterator location;
    DataHandle h;
    bool resolved;
    bool registered;
  };

} // namespace Arc

#endif // __ARC_DATAPOINTINDEX_H__
