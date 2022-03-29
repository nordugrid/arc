// -*- indent-tabs-mode: nil -*-

#ifndef __ARCDMCHTTP_DATAPOINTHTTP_H__
#define __ARCDMCHTTP_DATAPOINTHTTP_H__

#include <arc/Thread.h>
#include <arc/communication/ClientInterface.h>
#include <arc/data/DataPointDirect.h>

namespace ArcDMCHTTP {

using namespace Arc;

  class ChunkControl;

  /**
   * This class allows access through HTTP to remote resources. HTTP over SSL
   * (HTTPS) and HTTP over GSI (HTTPG) are also supported.
   *
   * This class is a loadable module and cannot be used directly. The DataHandle
   * class loads modules at runtime and should be used instead of this.
   */
  class DataPointHTTP
    : public DataPointDirect {
  public:
    DataPointHTTP(const URL& url, const UserConfig& usercfg, PluginArgument* parg);
    virtual ~DataPointHTTP();
    static Plugin* Instance(PluginArgument *arg);
    virtual bool SetURL(const URL& url);
    virtual DataStatus Check(bool check_meta);
    virtual DataStatus Remove();
    virtual DataStatus CreateDirectory(bool with_parents=false);
    virtual DataStatus Rename(const URL& url);
    virtual DataStatus Stat(FileInfo& file, DataPointInfoType verb = INFO_TYPE_ALL);
    virtual DataStatus List(std::list<FileInfo>& files, DataPointInfoType verb = INFO_TYPE_ALL);
    virtual DataStatus StartReading(DataBuffer& buffer);
    virtual DataStatus StartWriting(DataBuffer& buffer, DataCallback *space_cb = NULL);
    virtual DataStatus StopReading();
    virtual DataStatus StopWriting();
    virtual bool RequiresCredentials() const { return url.Protocol() != "http"; };
    virtual bool WriteOutOfOrder() const { return partial_write_allowed; };
  private:
    static void read_thread(void *arg);
    static bool read_single(void *arg);
    static void write_thread(void *arg);
    static bool write_single(void *arg);
    DataStatus do_stat_http(URL& curl, FileInfo& file);
    DataStatus do_stat_webdav(URL& curl, FileInfo& file);
    DataStatus do_list_webdav(URL& rurl, std::list<FileInfo>& files, DataPointInfoType verb);
    DataStatus makedir(const URL& dir);
    ClientHTTP* acquire_client(const URL& curl);
    ClientHTTP* acquire_new_client(const URL& curl);
    void release_client(const URL& curl, ClientHTTP* client);
    /// Convert HTTP return code to errno
    int http2errno(int http_code) const;
    static Logger logger;
    bool reading;
    bool writing;
    ChunkControl *chunks;
    std::multimap<std::string,ClientHTTP*> clients;
    SimpleCounter transfers_started;
    int transfers_tofinish;
    Glib::Mutex transfer_lock;
    Glib::Mutex clients_lock;
    bool partial_read_allowed;
    bool partial_write_allowed;
  };

} // namespace Arc

#endif // __ARCDMCHTTP_DATAPOINTHTTP_H__
