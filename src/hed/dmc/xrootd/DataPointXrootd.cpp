#include <XrdClient/XrdClient.hh>

#include <arc/StringConv.h>
#include <arc/data/DataBuffer.h>
#include <arc/CheckSum.h>

// "Error" macro defined here conflicts with same name macro in glib
// so have to include after glib includes
#include <XrdClient/XrdClientDebug.hh>

#include "DataPointXrootd.h"

namespace Arc {

  Logger DataPointXrootd::logger(Logger::getRootLogger(), "DataPoint.Xrootd");

  DataPointXrootd::DataPointXrootd(const URL& url, const UserConfig& usercfg)
    : DataPointDirect(url, usercfg),
      reading(false),
      writing(false) {
    client = new XrdClient(url.str().c_str());
    // set xrootd log level
    set_log_level();
  }

  DataPointXrootd::~DataPointXrootd() {
    StopReading();
    StopWriting();
    if (client) delete client;
  }

  Plugin* DataPointXrootd::Instance(PluginArgument *arg) {
    DataPointPluginArgument *dmcarg = dynamic_cast<DataPointPluginArgument*>(arg);
    if (!dmcarg)
      return NULL;
    if (((const URL &)(*dmcarg)).Protocol() != "root")
      return NULL;
    return new DataPointXrootd(*dmcarg, *dmcarg);
  }

  void DataPointXrootd::read_file_start(void* arg) {
    ((DataPointXrootd*)arg)->read_file();
  }

  void DataPointXrootd::read_file() {

    // TODO range reads
    bool do_cksum = true;
    unsigned long long int offset = 0;
    bool eof = false;

    for (;;) {
      /* 1. claim buffer */
      int h;
      unsigned int l;
      if (!buffer->for_read(h, l, true)) {
        /* failed to get buffer - must be error or request to exit */
        buffer->error_read(true);
        break;
      }
      if (buffer->error()) {
        buffer->is_read(h, 0, 0);
        break;
      }

      if (eof) {
        buffer->is_read(h, 0, 0);
        if(do_cksum) {
          for(std::list<CheckSum*>::iterator cksum = checksums.begin();
                    cksum != checksums.end(); ++cksum) {
            if(*cksum) (*cksum)->end();
          }
        }
        break;
      }
      /* 2. read */
      // it is an error to read past eof, so check if we are going to
      if (GetSize() - offset < l) {
        l = GetSize() - offset;
        eof = true;
        if (l == 0) {
          buffer->is_read(h, 0, 0);
          continue;
        }
      }
      logger.msg(DEBUG, "Reading %u bytes from byte %llu", l, offset);
      int res = client->Read((*(buffer))[h], offset, l);
      logger.msg(DEBUG, "Read %i bytes", res);
      if (res <= 0) { /* error */
        buffer->is_read(h, 0, 0);
        buffer->error_read(true);
        break;
      }

      if(do_cksum) {
        for(std::list<CheckSum*>::iterator cksum = checksums.begin();
                  cksum != checksums.end(); ++cksum) {
          if(*cksum) (*cksum)->add((*(buffer))[h], res);
        }
      }
      /* 3. announce */
      buffer->is_read(h, res, offset);
      offset += res;
    }
    client->Close();
    buffer->eof_read(true);
    transfer_cond.signal();
  }

  void DataPointXrootd::set_log_level() {
    // TODO xrootd lib logs to stderr - need to redirect to log file
    // for new data staging
    if (logger.getThreshold() == DEBUG)
      XrdClientDebug::Instance()->SetLevel(XrdClientDebug::kHIDEBUG);
    else if (logger.getThreshold() <= INFO)
      XrdClientDebug::Instance()->SetLevel(XrdClientDebug::kUSERDEBUG);
    else
      XrdClientDebug::Instance()->SetLevel(XrdClientDebug::kNODEBUG);
  }

  DataStatus DataPointXrootd::StartReading(DataBuffer& buf) {
    if (reading) return DataStatus::IsReadingError;
    if (writing) return DataStatus::IsWritingError;
    reading = true;

    // same client object cannot be opened twice, so create a new one here
    // in case it was used before
    if (client) {
      delete client;
      client = NULL;
    }
    client = new XrdClient(url.str().c_str());
    set_log_level();

    {
      CertEnvLocker env(usercfg);
      if (!client->Open(kXR_ur, kXR_open_read)) {
         logger.msg(ERROR, "Could not open file %s for reading", url.str());
         reading = false;
         return DataStatus::ReadStartError;
      }
    }
    // wait for open to complete
    if (!client->IsOpen_wait()) {
      logger.msg(ERROR, "Failed to open file %s", url.str());
      return DataStatus::ReadStartError;
    }

    // stat to find filesize if not already known
    if (GetSize() == (unsigned long long int)(-1)) {
      FileInfo file;
      DataPointInfoType info(INFO_TYPE_CONTENT);
      if (!Stat(file, info))
        return DataStatus::ReadStartError;
    }

    buffer = &buf;
    transfer_cond.reset();
    // create thread to maintain reading
    if(!CreateThreadFunction(&DataPointXrootd::read_file_start, this)) {
      client->Close();
      reading = false;
      return DataStatus::ReadStartError;
    }

    return DataStatus::Success;
  }

  DataStatus DataPointXrootd::StopReading() {
    if (!reading) return DataStatus::ReadStopError;
    reading = false;
    if (!buffer->eof_read()) {
      buffer->error_read(true);      /* trigger transfer error */
      client->Close();
    }
    transfer_cond.wait();         /* wait till reading thread exited */
    if (buffer->error_read())
      return DataStatus::ReadError;
    return DataStatus::Success;
  }


  DataStatus DataPointXrootd::StartWriting(DataBuffer& buffer,
                                      DataCallback *space_cb) {
    logger.msg(ERROR, "Writing to xrootd is not (yet) supported");
    return DataStatus::WriteError;
  }

  DataStatus DataPointXrootd::StopWriting() {
    return DataStatus::Success;
  }

  DataStatus DataPointXrootd::Check() {
    // check if file can be opened for reading
    {
      CertEnvLocker env(usercfg);
      if (!client->Open(kXR_ur, kXR_open_read)) {
         logger.msg(ERROR, "Could not open file %s", url.str());
         return DataStatus::CheckError;
      }
    }
    // wait for open to complete
    if (!client->IsOpen_wait()) {
      logger.msg(ERROR, "Failed to open file %s", url.str());
      return DataStatus::CheckError;
    }

    client->Close();
    return DataStatus::Success;
  }

  DataStatus DataPointXrootd::Stat(FileInfo& file, DataPointInfoType verb) {
    struct XrdClientStatInfo stinfo;
    bool already_opened = client->IsOpen();
    {
      CertEnvLocker env(usercfg);
      if (!already_opened && !client->Open(kXR_ur, kXR_open_read)) {
        logger.msg(ERROR, "Could not open file %s", url.str());
        return DataStatus::StatError;
      }
    }
    // wait for open to complete
    if (!client->IsOpen_wait()) {
      logger.msg(ERROR, "Failed to open file %s", url.str());
      return DataStatus::StatError;
    }

    if (!client->Stat(&stinfo)) {
      logger.msg(ERROR, "Could not stat file %s", url.str());
      if (!already_opened)
        client->Close();
      return DataStatus::StatError;
    }
    file.SetName(url.Path());
    file.SetMetaData("path", url.Path());

    file.SetType(FileInfo::file_type_file);
    file.SetMetaData("type", "file");

    file.SetSize(stinfo.size);
    file.SetMetaData("size", tostring(stinfo.size));

    file.SetCreated(stinfo.modtime);
    file.SetMetaData("mtime", tostring(stinfo.modtime));

    SetSize(file.GetSize());
    SetCreated(stinfo.modtime);

    if (!already_opened)
      client->Close();
    return DataStatus::Success;
  }

  DataStatus DataPointXrootd::List(std::list<FileInfo>& files, DataPointInfoType verb) {
    // cannot list directories so return same info as stat
    logger.msg(WARNING, "Cannot list directories with xrootd");
    FileInfo f;
    if (!Stat(f, verb))
      return DataStatus::ListError;
    files.push_back(f);
    return DataStatus::Success;
  }

  DataStatus DataPointXrootd::Remove() {
    logger.msg(ERROR, "Cannot remove files through xrootd");
    return DataStatus::DeleteError;
  }

} // namespace Arc


Arc::PluginDescriptor PLUGINS_TABLE_NAME[] = {
  { "xrootd", "HED:DMC", 0, &Arc::DataPointXrootd::Instance },
  { NULL, NULL, 0, NULL }
};
