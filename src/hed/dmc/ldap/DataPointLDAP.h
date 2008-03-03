#ifndef __ARC_DATAPOINTLDAP_H__
#define __ARC_DATAPOINTLDAP_H__

#include <string>
#include <list>

#include <arc/data/DataPointDirect.h>

#include <arc/Logger.h>
#include <arc/Thread.h>
#include <arc/XMLNode.h>

namespace Arc {

  class DataPointLDAP : public DataPointDirect {
   public:
    DataPointLDAP(const URL& url);
    virtual ~DataPointLDAP();
    virtual bool start_reading(DataBufferPar& buffer);
    virtual bool start_writing(DataBufferPar& buffer,
                               DataCallback *space_cb = NULL);
    virtual bool stop_reading();
    virtual bool stop_writing();
    virtual bool check();
    virtual bool remove();
    virtual bool list_files(std::list<FileInfo>& files, bool resolve = true);
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
