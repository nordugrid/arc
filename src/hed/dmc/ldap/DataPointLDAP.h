#ifndef __ARC_DATAPOINTLDAP_H__
#define __ARC_DATAPOINTLDAP_H__

#include <string>
#include <list>

#include <arc/data/DataPointDirect.h>

#include <arc/Logger.h>
#include <arc/Thread.h>
#include <arc/XMLNode.h>

namespace Arc {

  class DataPointLDAP
    : public DataPointDirect {
  public:
    DataPointLDAP(const URL& url);
    virtual ~DataPointLDAP();
    virtual DataStatus StartReading(DataBufferPar& buffer);
    virtual DataStatus StartWriting(DataBufferPar& buffer,
				    DataCallback *space_cb = NULL);
    virtual DataStatus StopReading();
    virtual DataStatus StopWriting();
    virtual DataStatus Check();
    virtual DataStatus Remove();
    virtual DataStatus ListFiles(std::list<FileInfo>& files,
				 bool resolve = true);
  private:
    XMLNode node;
    XMLNode entry;
    static void CallBack(const std::string& attr,
			 const std::string& value, void *arg);
    static void ReadThread(void *arg);
    static Logger logger;
  };

} // namespace Arc

#endif // __ARC_DATAPOINTLDAP_H__
