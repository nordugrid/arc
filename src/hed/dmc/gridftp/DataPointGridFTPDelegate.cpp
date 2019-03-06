// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <openssl/ssl.h>

#include <arc/Logger.h>
#include <arc/StringConv.h>
#include <arc/UserConfig.h>
#include <arc/data/DataBuffer.h>
#include <arc/CheckSum.h>
#include <arc/Run.h>
#include <arc/ArcLocation.h>

#include "Communication.h"

#include "DataPointGridFTPDelegate.h"

namespace ArcDMCGridFTP {

  using namespace Arc;

  Logger DataPointGridFTPDelegate::logger(Logger::getRootLogger(), "DataPoint.GridFTP");


  DataStatus DataPointGridFTPDelegate::StartCommand(Arc::CountedPointer<Arc::Run>& run, std::list<std::string>& argv, DataBuffer& buf, DataStatus::DataStatusType errCode) {
    argv.push_front(Arc::tostring(buf.buffer_size()));
    argv.push_front("-b");
    argv.push_front(Arc::tostring(range_end));
    argv.push_front("-E");
    argv.push_front(Arc::tostring(range_start));
    argv.push_front("-S");
    return StartCommand(run, argv, errCode);
  }

  DataStatus DataPointGridFTPDelegate::StartCommand(Arc::CountedPointer<Arc::Run>& run, std::list<std::string>& argv, DataStatus::DataStatusType errCode) {
    argv.push_front(Arc::tostring(force_passive));
    argv.push_front("-p");
    argv.push_front(Arc::tostring(force_secure));
    argv.push_front("-s");
    argv.push_front(Arc::level_to_string(logger.getThreshold()));
    argv.push_front("-V");
    LogFormat format = ShortFormat; // Header is then added while redirecting stderr
    argv.push_front(Arc::tostring(format));
    argv.push_front("-F");
    argv.push_front(Arc::ArcLocation::Get()+G_DIR_SEPARATOR_S+PKGLIBSUBDIR+G_DIR_SEPARATOR_S+"arc-dmcgridftp");
    run = new Run(argv);
    run->KeepStdin(false);
    run->KeepStdout(false);
    run->KeepStderr(false);
    run->AssignStderr(log_redirect);
    if(!run->Start()) {
      return DataStatus(errCode, "Failed to start helper process for "+url.plainstr());
    }
    if(!OutEntry(*run, 1000*usercfg.Timeout(), usercfg)) {
      return DataStatus(errCode, "Failed to pass configuration to helper process for "+url.plainstr());
    }
    return DataStatus::Success;
  }


  DataStatus DataPointGridFTPDelegate::EndCommand(Arc::CountedPointer<Arc::Run>& run, DataStatus::DataStatusType errCode) {
    char tag = InTag(*run, 1000*usercfg.Timeout());
    return EndCommand(run, errCode, tag);
  }

  DataStatus DataPointGridFTPDelegate::EndCommand(Arc::CountedPointer<Arc::Run>& run, DataStatus::DataStatusType errCode, char tag) {
    if(tag != DataStatusTag) {
      return DataStatus(errCode, "Unexpected data status tag from helper process for "+url.plainstr());
    }
    DataStatus result;
    if(!InEntry(*run, 1000*usercfg.Timeout(), result)) {
      return DataStatus(errCode, "Failed to read data status from helper process for "+url.plainstr());
    }
    if(!run->Wait(1000*usercfg.Timeout())) {
      return DataStatus(errCode, EARCREQUESTTIMEOUT, "Timeout waiting for helper process for "+url.plainstr());
    }
    if(run->Result() != 0) {
      return DataStatus(errCode, run->Result(), "Failed helper process for "+url.plainstr());
    }
    if(!result) failure_code = result;
    return result;
  }


  DataStatus DataPointGridFTPDelegate::Check(bool check_meta) {
    if (reading) return DataStatus::IsReadingError;
    if (writing) return DataStatus::IsWritingError;

    std::list<std::string> argv;
    argv.push_back("check");
    argv.push_back(url.fullstr());
    Arc::CountedPointer<Arc::Run> run;
    DataStatus result = StartCommand(run, argv, DataStatus::CheckError);
    if(!result) return result;
    // No additional information expected
    result = EndCommand(run, DataStatus::CheckError);
    if(!result) return result;
    if(check_meta) {
      FileInfo file;
      if(Stat(file, DataPoint::INFO_TYPE_CONTENT)) {
        if(file.CheckModified()) SetModified(file.GetModified());
        if(file.CheckSize()) SetSize(file.GetSize());
      }
    }
    return DataStatus::Success;
  }

  DataStatus DataPointGridFTPDelegate::Remove() {
    if (reading) return DataStatus::IsReadingError;
    if (writing) return DataStatus::IsWritingError;

    std::list<std::string> argv;
    argv.push_back("remove");
    argv.push_back(url.fullstr());
    Arc::CountedPointer<Arc::Run> run;
    DataStatus result = StartCommand(run, argv, DataStatus::DeleteError);
    if(!result) return result;
    // No additional information expected
    result = EndCommand(run, DataStatus::DeleteError);
    if(!result) return result;
    return DataStatus::Success;
  }

  DataStatus DataPointGridFTPDelegate::CreateDirectory(bool with_parents) {
    if (reading) return DataStatus::IsReadingError;
    if (writing) return DataStatus::IsWritingError;

    std::list<std::string> argv;
    if(with_parents) {
      argv.push_back("mkdirr");
    } else {
      argv.push_back("mkdir");
    }
    argv.push_back(url.fullstr());
    Arc::CountedPointer<Arc::Run> run;
    DataStatus result = StartCommand(run, argv, DataStatus::CreateDirectoryError);
    if(!result) return result;
    // No additional information expected
    result = EndCommand(run, DataStatus::CreateDirectoryError);
    if(!result) return result;
    return DataStatus::Success;
  }

  DataStatus DataPointGridFTPDelegate::StartReading(DataBuffer& buf) {
    if (reading) return DataStatus::IsReadingError;
    if (writing) return DataStatus::IsWritingError;
    reading = true;
    buffer = &buf;
    logger.msg(VERBOSE, "start_reading_ftp");
    cond.reset();
    data_status = DataStatus::Success;
    std::list<std::string> argv;
    argv.push_back("read");
    argv.push_back(url.fullstr());
    DataStatus result = StartCommand(ftp_run, argv, buf, DataStatus::ReadStartError);
    if(!result) {
      ftp_run = NULL;
      logger.msg(VERBOSE, "start_reading_ftp: helper start failed");
      buffer->error_read(true);
      reading = false;
      return result;
    }
    // Start thread for passing data
    if(!Arc::CreateThreadFunction(&ftp_read_thread, this)) {
      ftp_run = NULL;
      logger.msg(VERBOSE, "start_reading_ftp: thread create failed");
      buffer->error_read(true);
      reading = false;
      return DataStatus(DataStatus::ReadStartError, "Failed to create new thread");
    }
    return DataStatus::Success;
  }

  DataStatus DataPointGridFTPDelegate::StopReading() {
    if (!reading) return DataStatus::ReadStopError;
    reading = false;

    if(!ftp_run) return DataStatus::Success; // never started? already stopped?

    // If error in buffer then write thread have already called abort
    if(buffer && !buffer->eof_read() && !buffer->error()) { // otherwise it will exit itself
      logger.msg(VERBOSE, "StopWriting: aborting connection");
      buffer->error_read(true);
    }
    ftp_run->Kill(1); // kill anyway - it won't get worse
    // Waiting for data transfer thread to finish
    logger.msg(VERBOSE, "stop_reading_ftp: waiting for transfer to finish");
    cond.wait();
    ftp_run = NULL;
    logger.msg(VERBOSE, "stop_reading_ftp: exiting: %s", url.plainstr());
    return data_status;
  }

  void DataPointGridFTPDelegate::ftp_read_thread(void *arg) {
    DataPointGridFTPDelegate *it = (DataPointGridFTPDelegate*)arg;
    if(!it) return;
    Arc::CountedPointer<Run> run(it->ftp_run);
    int h;
    unsigned int l;
    logger.msg(INFO, "ftp_read_thread: get and register buffers");
    DataChunkExtBuffer chunkReader;
    char tag;
    for (;;) {
      tag = ErrorTag;
      if (it->buffer->eof_read()) break;
      if (!it->buffer->for_read(h, l, true)) { // eof or error
        if (it->buffer->error()) { // error -> abort reading
          logger.msg(VERBOSE, "ftp_read_thread: for_read failed - aborting: %s",
                     it->url.plainstr());
        }
        break;
      }
      if(chunkReader.complete()) {
        tag = InTag(*run, 1000 * it->usercfg.Timeout());
        if(tag != DataChunkTag) {
          it->buffer->is_read(h, 0, 0);
          break;
        }
      }    
      tag = ErrorTag;
      unsigned long long int offset = 0;
      unsigned long long int size = l;
      if(!chunkReader.read(*run, 1000 * it->usercfg.Timeout(), (*(it->buffer))[h], offset, size)) {
        it->buffer->is_read(h, 0, 0);
        it->buffer->error_read(true);
        break;
      }
      it->buffer->is_read(h, size, offset);
      // let tag catch it - if(size == 0) break; // eof
    }
    logger.msg(VERBOSE, "ftp_read_thread: exiting");
    it->data_status = it->EndCommand(run, DataStatus::ReadError, tag);
    if(!it->data_status) it->buffer->error_read(true);
    it->buffer->eof_read(true);
    it->cond.signal();
  }

  DataStatus DataPointGridFTPDelegate::StartWriting(DataBuffer& buf, DataCallback*) {
    if (reading) return DataStatus::IsReadingError;
    if (writing) return DataStatus::IsWritingError;
    writing = true;
    buffer = &buf;
    // size of file first 
    cond.reset();
    data_status = DataStatus::Success;
    std::list<std::string> argv;
    argv.push_back("write");
    argv.push_back(url.fullstr());
    DataStatus result = StartCommand(ftp_run, argv, buf, DataStatus::WriteStartError);
    if(!result) {
      ftp_run = NULL;
      logger.msg(VERBOSE, "start_writing_ftp: helper start failed");
      buffer->error_write(true);
      writing = false;
      return result;
    }
    // Start thread for passing data
    if(!Arc::CreateThreadFunction(&ftp_write_thread, this)) {
      ftp_run = NULL;
      logger.msg(VERBOSE, "start_writing_ftp: thread create failed");
      buffer->error_write(true);
      writing = false;
      return DataStatus(DataStatus::WriteStartError, "Failed to create new thread");
    }
    return DataStatus::Success;
  }

  DataStatus DataPointGridFTPDelegate::StopWriting() {
    if (!writing) return DataStatus::WriteStopError;
    writing = false;
    if(!ftp_run) return DataStatus::Success; // never started? already stopped?
    // If error in buffer then write thread have already called abort
    if(buffer && !buffer->eof_write() && !buffer->error()) { // otherwise it will exit itself
      logger.msg(VERBOSE, "StopWriting: aborting connection");
      buffer->error_write(true);
      ftp_run->Kill(1);
    }
    // Waiting for data transfer thread to finish
    cond.wait();
    ftp_run = NULL;
    // checksum verification
    const CheckSum * calc_sum = buffer->checksum_object();
    if (data_status && !buffer->error() && calc_sum && *calc_sum && buffer->checksum_valid()) {
      char buf[100];
      calc_sum->print(buf,100);
      std::string csum(buf);
      if (csum.find(':') != std::string::npos && csum.substr(0, csum.find(':')) == DefaultCheckSum()) {
        logger.msg(VERBOSE, "StopWriting: Calculated checksum %s", csum);
        if(additional_checks) {
          // list checksum and compare
          // note: not all implementations support checksum
          logger.msg(DEBUG, "StopWriting: "
                            "looking for checksum of %s", url.plainstr());
          FileInfo info;
          if(Stat(info, DataPoint::INFO_TYPE_CONTENT)) {
            if(info.CheckCheckSum()) {
              if(csum.length() != info.GetCheckSum().length()) {
                // Some buggy Globus servers return a different type of checksum to the one requested
                logger.msg(WARNING, "Checksum type returned by server is different to requested type, cannot compare");
              } else if (csum == info.GetCheckSum()) {
                logger.msg(INFO, "Calculated checksum %s matches checksum reported by server", csum);
                SetCheckSum(csum);
              } else {
                logger.msg(VERBOSE, "Checksum mismatch between calculated checksum %s and checksum reported by server %s",
                           csum, info.GetCheckSum());
                data_status = DataStatus(DataStatus::TransferError, EARCCHECKSUM,
                                         "Checksum mismatch between calculated and reported checksums");
              }
            } else {
              logger.msg(INFO, "No checksum information possible");
            }
          }
        }
      }
    }
    return data_status;
  }

  void DataPointGridFTPDelegate::ftp_write_thread(void *arg) {
    DataPointGridFTPDelegate *it = (DataPointGridFTPDelegate*)arg;
    if(!it) return;
    Arc::CountedPointer<Run> run(it->ftp_run);
    DataBuffer& buffer(*(it->buffer));
    bool out_failed = false;
    if(run) {
      int timeout = it->usercfg.Timeout()*1000;
      logger.msg(INFO, "ftp_write_thread: get and pass buffers");
      for (;;) {
        int h;
        unsigned int l;
        unsigned long long int o;
        if (!buffer.for_write(h, l, o, true)) {
          if (buffer.error()) {
            logger.msg(VERBOSE, "ftp_write_thread: for_write failed - aborting");
            buffer.error_write(true);
            break;
          }
          logger.msg(VERBOSE, "ftp_write_thread: for_write eof");
          // no buffers and no errors - must be pure eof
          o = buffer.eof_position();
          DataChunkExtBuffer dc;
          if((!OutTag(*run, timeout, DataChunkTag)) || (!dc.write(*run, timeout, NULL, o, 0))) {
            out_failed = true;
            break;
          }
          buffer.eof_write(true);
          break;
        }
        if(l > 0) {
          DataChunkExtBuffer dc;
          if((!OutTag(*run, timeout, DataChunkTag)) || (!dc.write(*run, timeout, buffer[h], o, l))) {
            logger.msg(VERBOSE, "ftp_write_thread: out failed - aborting");
            buffer.is_notwritten(h);
            out_failed = true;
            break;
          }
        }
        buffer.is_written(h);
      }
    }
    logger.msg(VERBOSE, "ftp_write_thread: exiting");
    if(out_failed) {
      buffer.error_write(true);
      // Communication with helper failed but may still read status
      it->data_status = it->EndCommand(run, DataStatus::WriteError);
    } else if(buffer.error_write()) {
      // Report generic error
      it->data_status = DataStatus::WriteError;
    } else {
      // So far so good - read status
      it->data_status = it->EndCommand(run, DataStatus::WriteError);
    }
    it->cond.signal(); // Report to control thread that data transfer thread finished
  }

  DataStatus DataPointGridFTPDelegate::Stat(FileInfo& file, DataPoint::DataPointInfoType verb) {
    if (reading) return DataStatus::IsReadingError;
    if (writing) return DataStatus::IsWritingError;

    std::list<std::string> argv;
    argv.push_back("stat");
    argv.push_back(url.fullstr());
    argv.push_back(Arc::tostring(verb));
    Arc::CountedPointer<Arc::Run> run;
    DataStatus result = StartCommand(run, argv, DataStatus::StatError);
    if(!result) return result;
    // Expecting one FileInfo
    bool file_is_available = false;
    char tag = InTag(*run, 1000*usercfg.Timeout());
    if(tag == FileInfoTag) { // Expected
      if(InEntry(*run, 1000*usercfg.Timeout(), file)) {
        file_is_available = true;
        // Now expecting end of command
        result = EndCommand(run, DataStatus::StatError);
      } else {
        result = DataStatus(DataStatus::StatError, "Failed to read result of helper process for "+url.plainstr());
      }
    } else { // May be error reported
      result = EndCommand(run, DataStatus::StatError, tag);
    };
    if(!result) return result; 
    if(!file_is_available) return DataStatus(DataStatus::StatError, "Failed to stat " + url.plainstr());
    return DataStatus::Success;
  }

  DataStatus DataPointGridFTPDelegate::List(std::list<FileInfo>& files, DataPoint::DataPointInfoType verb) {
    if (reading) return DataStatus::IsReadingError;
    if (writing) return DataStatus::IsWritingError;

    std::list<std::string> argv;
    argv.push_back("list");
    argv.push_back(url.fullstr());
    argv.push_back(Arc::tostring(verb));
    Arc::CountedPointer<Arc::Run> run;
    DataStatus result = StartCommand(run, argv, DataStatus::ListError);
    if(!result) return result;
    // Expecting [0-inf) FileInfo
    char tag = InTag(*run, 1000*usercfg.Timeout());
    while(tag == FileInfoTag) {
      Arc::FileInfo file;
      if(!InEntry(*run, 1000*usercfg.Timeout(), file)) {
        // Communication is broken - can't proceed with reading
        result = DataStatus(DataStatus::ListError, "Failed to read helper result for "+url.plainstr());
        break;
      }
      file.SetName(file.GetLastName()); // relative name is expected historically
      files.push_back(file);
      tag = InTag(*run, 1000*usercfg.Timeout());
    }
    if(result) {
      result = EndCommand(run, DataStatus::ListError, tag);
    }
    if(!result) return result; 
    return DataStatus::Success;
  }

  DataStatus DataPointGridFTPDelegate::Rename(const URL& newurl) {
    if (reading) return DataStatus::IsReadingError;
    if (writing) return DataStatus::IsWritingError;

    std::list<std::string> argv;
    argv.push_back("rename");
    argv.push_back(url.fullstr());
    argv.push_back(newurl.fullstr());
    Arc::CountedPointer<Arc::Run> run;
    DataStatus result = StartCommand(run, argv, DataStatus::RenameError);
    if(!result) return result;
    result = EndCommand(run, DataStatus::ListError);
    if(!result) return result; 
    return DataStatus::Success;
  }

  DataPointGridFTPDelegate::DataPointGridFTPDelegate(const URL& url, const UserConfig& usercfg, PluginArgument* parg)
    : DataPointDirect(url, usercfg, parg),
      reading(false),
      writing(false),
      ftp_run() {
    is_secure = false;
    if (url.Protocol() == "gsiftp") is_secure = true;
    ftp_threads = 1;
    if (allow_out_of_order) {
      ftp_threads = stringtoi(url.Option("threads"));
      if (ftp_threads < 1)
        ftp_threads = 1;
      if (ftp_threads > MAX_PARALLEL_STREAMS)
        ftp_threads = MAX_PARALLEL_STREAMS;
    }
    autodir = additional_checks;
    std::string autodir_s = url.Option("autodir");
    if(autodir_s == "yes") {
      autodir = true;
    } else if(autodir_s == "no") {
      autodir = false;
    }
  }

  DataPointGridFTPDelegate::~DataPointGridFTPDelegate() {
    StopReading();
    StopWriting();
  }

  Plugin* DataPointGridFTPDelegate::Instance(PluginArgument *arg) {
    DataPointPluginArgument *dmcarg = dynamic_cast<DataPointPluginArgument*>(arg);
    if (!dmcarg) return NULL;
    if (((const URL&)(*dmcarg)).Protocol() != "gsiftp" &&
        ((const URL&)(*dmcarg)).Protocol() != "ftp") {
      return NULL;
    }
    return new DataPointGridFTPDelegate(*dmcarg, *dmcarg, dmcarg);
  }

  bool DataPointGridFTPDelegate::WriteOutOfOrder() {
    return true;
  }

  bool DataPointGridFTPDelegate::ProvidesMeta() const {
    return true;
  }

  const std::string DataPointGridFTPDelegate::DefaultCheckSum() const {
    // no way to know which checksum is used for each file, so hard-code adler32 for now
    return std::string("adler32");
  }

  bool DataPointGridFTPDelegate::RequiresCredentials() const {
    return is_secure;
  }

  bool DataPointGridFTPDelegate::SetURL(const URL& u) {
    if ((u.Protocol() != "gsiftp") && (u.Protocol() != "ftp")) {
      return false;
    }
    if (u.Host() != url.Host()) {
      return false;
    }
    // Globus FTP handle allows changing url completely
    url = u;
    if(triesleft < 1) triesleft = 1;
    ResetMeta();
    return true;
  }

  std::string::size_type const DataPointGridFTPDelegate::LogRedirect::level_size_max_ = 32;
  std::string::size_type const DataPointGridFTPDelegate::LogRedirect::buffer_size_max_ = 4096;

  void DataPointGridFTPDelegate::LogRedirect::Append(char const* data, unsigned int size) {
    while(size >= 0) {
      char const* sep = (char const*)memchr(data, '\n', size);
      if (sep == NULL) break;
      if(buffer_.length() < buffer_size_max_) buffer_.append(data,sep-data);
      Flush();
      size -= sep-data+1;
      data = sep+1;
    }
    if (size > 0) buffer_.append(data,size);
  }

  void DataPointGridFTPDelegate::LogRedirect::Flush() {
    if(!buffer_.empty()) {
      // I could not find any better method for recovering message level
      std::string::size_type dsep = buffer_.find(':');
      if((dsep != std::string::npos) &&
         (dsep < level_size_max_) &&
         (string_to_level(buffer_.substr(0,dsep),level_))) {
        dsep += 1;
      } else {
        dsep = 0;
      }
      logger.msg(level_, "%s", buffer_.c_str()+dsep);
      buffer_.clear();
    }
  }

} // namespace ArcDMCGridFTP

extern Arc::PluginDescriptor const ARC_PLUGINS_TABLE_NAME[] = {
  { "gsiftp", "HED:DMC", "FTP or FTP with GSI security", 0, &ArcDMCGridFTP::DataPointGridFTPDelegate::Instance },
  { NULL, NULL, NULL, 0, NULL }
};

extern "C" {
  void ARC_MODULE_CONSTRUCTOR_NAME(Glib::Module* module, Arc::ModuleManager* manager) {
  }
}

