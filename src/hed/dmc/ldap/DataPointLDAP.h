// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_DATAPOINTLDAP_H__
#define __ARC_DATAPOINTLDAP_H__

#include <list>
#include <string>

#include <arc/Logger.h>
#include <arc/Thread.h>
#include <arc/XMLNode.h>
#include <arc/data/DataPointDirect.h>

namespace ArcDMCLDAP {

  using namespace Arc;

  /**
   * LDAP is used in grids mainly to store information about grid services
   * or resources rather than to store data itself. This class allows access
   * to LDAP data through the same interface as other grid resources.
   *
   * This class is a loadable module and cannot be used directly. The DataHandle
   * class loads modules at runtime and should be used instead of this.
   */
  class DataPointLDAP
    : public DataPointDirect {
  public:
    DataPointLDAP(const URL& url, const UserConfig& usercfg, const std::string& transfer_url, PluginArgument* parg);
    virtual ~DataPointLDAP();
    static Plugin* Instance(PluginArgument *arg);
    virtual DataStatus StartReading(DataBuffer& buffer);
    virtual DataStatus StartWriting(DataBuffer& buffer,
                                    DataCallback *space_cb = NULL);
    virtual DataStatus StopReading();
    virtual DataStatus StopWriting();
    virtual DataStatus Check(bool check_meta);
    virtual DataStatus Remove();
    virtual DataStatus CreateDirectory(bool with_parents=false) { return DataStatus(DataStatus::UnimplementedError, EOPNOTSUPP); };
    virtual DataStatus Rename(const URL& newurl) { return DataStatus(DataStatus::UnimplementedError, EOPNOTSUPP); };
    virtual DataStatus Stat(FileInfo& file, DataPoint::DataPointInfoType verb);
    virtual DataStatus List(std::list<FileInfo>& file, DataPoint::DataPointInfoType verb);
    virtual bool RequiresCredentials() const { return false; };
  private:
    XMLNode node;
    XMLNode entry;
    std::map<std::string, XMLNode> dn_cache;
    SimpleCounter thread_cnt;
    static void CallBack(const std::string& attr,
                         const std::string& value, void *arg);
    static void ReadThread(void *arg);
    static Logger logger;
  };

} // namespace ArcDMCLDAP

#endif // __ARC_DATAPOINTLDAP_H__
