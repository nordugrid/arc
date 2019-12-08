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
#include "DataExternalComm.h"

#include "DataPointDelegate.h"

namespace Arc {


  char const * DataPointDelegate::ReadCommand = "read";
  char const * DataPointDelegate::WriteCommand = "write";
  char const * DataPointDelegate::MkdirCommand = "mkdir";
  char const * DataPointDelegate::MkdirRecursiveCommand = "mkdirr";
  char const * DataPointDelegate::CheckCommand = "check";
  char const * DataPointDelegate::RemoveCommand = "remove";
  char const * DataPointDelegate::StatCommand = "stat";
  char const * DataPointDelegate::ListCommand = "list";
  char const * DataPointDelegate::RenameCommand = "rename";
  char const * DataPointDelegate::TransferFromCommand = "transferfrom";
  char const * DataPointDelegate::TransferToCommand = "transferto";
  char const * DataPointDelegate::Transfer3rdCommand = "transfer3";

  Logger DataPointDelegate::logger(Logger::getRootLogger(), "DataPoint.Delegate");

  DataStatus DataPointDelegate::StartCommand(Arc::CountedPointer<Arc::Run>& run, std::list<std::string>& argv, DataBuffer& buf, DataStatus::DataStatusType errCode) {
    argv.push_front(Arc::tostring(buf.buffer_size()));
    argv.push_front("-b");
    argv.push_front(Arc::tostring(range_end));
    argv.push_front("-E");
    argv.push_front(Arc::tostring(range_start));
    argv.push_front("-S");
    return StartCommand(run, argv, errCode);
  }

  static std::string ListToString(std::list<std::string> const & values) {
    std::string value;
    for(std::list<std::string>::const_iterator valIt = values.begin(); valIt != values.end(); ++valIt) {
      value += " ";
      value += *valIt;
    }
    return value;
  }

  DataStatus DataPointDelegate::StartCommand(Arc::CountedPointer<Arc::Run>& run, std::list<std::string>& argv, DataStatus::DataStatusType errCode) {
    argv.push_front(Arc::tostring(force_passive));
    argv.push_front("-p");
    argv.push_front(Arc::tostring(force_secure));
    argv.push_front("-s");
    argv.push_front(Arc::level_to_string(logger.getThreshold()));
    argv.push_front("-V");
    LogFormat format = ShortFormat; // Header is then added while redirecting stderr
    argv.push_front(Arc::tostring(format));
    argv.push_front("-F");
    argv.push_front(exec_path);
    run = new Run(argv);
    run->KeepStdin(false);
    run->KeepStdout(false);
    run->KeepStderr(false);
    run->AssignStderr(log_redirect);
    logger.msg(DEBUG, "Starting hepler process: %s", ListToString(argv));
    if(!run->Start()) {
      return DataStatus(errCode, "Failed to start helper process for "+url.plainstr());
    }
    if(!DataExternalComm::OutEntry(*run, 1000*usercfg.Timeout(), usercfg)) {
      return DataStatus(errCode, "Failed to pass configuration to helper process for "+url.plainstr());
    }
    return DataStatus::Success;
  }


  DataStatus DataPointDelegate::EndCommand(Arc::CountedPointer<Arc::Run>& run, DataStatus::DataStatusType errCode) {
    char tag = DataExternalComm::InTag(*run, 1000*usercfg.Timeout());
    return EndCommand(run, errCode, tag);
  }

  DataStatus DataPointDelegate::EndCommand(Arc::CountedPointer<Arc::Run>& run, DataStatus::DataStatusType errCode, char tag) {
    if(tag == DataExternalComm::ErrorTag) {
      return DataStatus(errCode, "Comunication error while waiting for data status from helper process for "+url.plainstr());
    }
    if(tag != DataExternalComm::DataStatusTag) {
      return DataStatus(errCode, "Unexpected tag while waiting for data status from helper process for "+url.plainstr());
    }
    DataStatus result;
    if(!DataExternalComm::InEntry(*run, 1000*usercfg.Timeout(), result)) {
      return DataStatus(errCode, "Failed to read data status from helper process for "+url.plainstr());
    }
    if(!result) failure_code = result;
    // At this point we have transfer result obtained from external process.
    // Now whatever error we get from the process itself it is less relevant than transfer result.
    // Hence wait for proces to exit. But do not do it for too long as process may hang due to internal bugs.
    if(!run->Wait(10)) {  // 10 seconds should be enough for exiting process to fully exit
      // We do not want processes hanging around.
      run->Kill(1);
    }
    // It is probably safe to ignore exit code of the external program. run->Result()
    return result;
  }


  DataStatus DataPointDelegate::Check(bool check_meta) {
    if (reading) return DataStatus::IsReadingError;
    if (writing) return DataStatus::IsWritingError;

    std::list<std::string> argv(additional_args);
    argv.push_back(CheckCommand);
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

  DataStatus DataPointDelegate::Remove() {
    if (reading) return DataStatus::IsReadingError;
    if (writing) return DataStatus::IsWritingError;

    std::list<std::string> argv(additional_args);
    argv.push_back(RemoveCommand);
    argv.push_back(url.fullstr());
    Arc::CountedPointer<Arc::Run> run;
    DataStatus result = StartCommand(run, argv, DataStatus::DeleteError);
    if(!result) return result;
    // No additional information expected
    result = EndCommand(run, DataStatus::DeleteError);
    if(!result) return result;
    return DataStatus::Success;
  }

  DataStatus DataPointDelegate::CreateDirectory(bool with_parents) {
    if (reading) return DataStatus::IsReadingError;
    if (writing) return DataStatus::IsWritingError;

    std::list<std::string> argv(additional_args);
    if(with_parents) {
      argv.push_back(MkdirRecursiveCommand);
    } else {
      argv.push_back(MkdirCommand);
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

  DataStatus DataPointDelegate::StartReading(DataBuffer& buf) {
    if (reading) return DataStatus::IsReadingError;
    if (writing) return DataStatus::IsWritingError;
    reading = true;
    buffer = &buf;
    logger.msg(VERBOSE, "start_reading");
    cond.reset();
    data_status = DataStatus::Success;
    std::list<std::string> argv(additional_args);
    argv.push_back(ReadCommand);
    argv.push_back(url.fullstr());
    DataStatus result = StartCommand(helper_run, argv, buf, DataStatus::ReadStartError);
    if(!result) {
      helper_run = NULL;
      logger.msg(VERBOSE, "start_reading: helper start failed");
      buffer->error_read(true);
      reading = false;
      return result;
    }
    // Start thread for passing data
    if(!Arc::CreateThreadFunction(&read_thread, this)) {
      helper_run = NULL;
      logger.msg(VERBOSE, "start_reading: thread create failed");
      buffer->error_read(true);
      reading = false;
      return DataStatus(DataStatus::ReadStartError, "Failed to create new thread");
    }
    return DataStatus::Success;
  }

  DataStatus DataPointDelegate::StopReading() {
    if (!reading) return DataStatus::ReadStopError;
    reading = false;

    if(!helper_run) return DataStatus::Success; // never started? already stopped?

    // If error in buffer then write thread have already called abort
    if(buffer && !buffer->eof_read() && !buffer->error()) { // otherwise it will exit itself
      logger.msg(VERBOSE, "StopReading: aborting connection");
      buffer->error_read(true);
    }
    helper_run->Kill(1); // kill anyway - it won't get worse
    // Waiting for data transfer thread to finish
    logger.msg(VERBOSE, "stop_reading: waiting for transfer to finish");
    cond.wait();
    helper_run = NULL;
    logger.msg(VERBOSE, "stop_reading: exiting: %s", url.plainstr());
    return data_status;
  }

  void DataPointDelegate::read_thread(void *arg) {
    DataPointDelegate *it = (DataPointDelegate*)arg;
    if(!it) return;
    Arc::CountedPointer<Run> run(it->helper_run);
    int h;
    unsigned int l;
    logger.msg(INFO, "read_thread: get and register buffers");
    DataExternalComm::DataChunkExtBuffer chunkReader;
    char tag;
    for (;;) {
      tag = DataExternalComm::ErrorTag;
      if (it->buffer->eof_read()) break;
      if (!it->buffer->for_read(h, l, true)) { // eof or error
        if (it->buffer->error()) { // error -> abort reading
          logger.msg(VERBOSE, "read_thread: for_read failed - aborting: %s", it->url.plainstr());
        }
        break;
      }
      if(chunkReader.complete()) {
        // No timeout here. Timeouts are implemented externally in DataMover through call to StopReading().
        tag = DataExternalComm::InTag(*run, -1);
        if(tag != DataExternalComm::DataChunkTag) {
          logger.msg(DEBUG, "read_thread: non-data tag '%c' from external process - leaving: %s", tag, it->url.plainstr());
          it->buffer->is_read(h, 0, 0);
          break;
        }
      }    
      tag = DataExternalComm::ErrorTag;
      unsigned long long int offset = 0;
      unsigned long long int size = l;
      if(!chunkReader.read(*run, 1000 * it->usercfg.Timeout(), (*(it->buffer))[h], offset, size)) {
        logger.msg(VERBOSE, "read_thread: data read error from external process - aborting: %s", it->url.plainstr());
        it->buffer->is_read(h, 0, 0);
        it->buffer->error_read(true);
        break;
      }
      it->buffer->is_read(h, size, offset);
      // let tag catch it - if(size == 0) break; // eof
    }
    logger.msg(VERBOSE, "read_thread: exiting");
    it->data_status = it->EndCommand(run, DataStatus::ReadError, tag);
    if(!it->data_status) it->buffer->error_read(true);
    it->buffer->eof_read(true);
    it->cond.signal();
  }

  DataStatus DataPointDelegate::StartWriting(DataBuffer& buf, DataCallback*) {
    if (reading) return DataStatus::IsReadingError;
    if (writing) return DataStatus::IsWritingError;
    writing = true;
    buffer = &buf;
    // size of file first 
    cond.reset();
    data_status = DataStatus::Success;
    std::list<std::string> argv(additional_args);
    argv.push_back(WriteCommand);
    argv.push_back(url.fullstr());
    DataStatus result = StartCommand(helper_run, argv, buf, DataStatus::WriteStartError);
    if(!result) {
      helper_run = NULL;
      logger.msg(VERBOSE, "start_writing_ftp: helper start failed");
      buffer->error_write(true);
      writing = false;
      return result;
    }
    // Start thread for passing data
    if(!Arc::CreateThreadFunction(&write_thread, this)) {
      helper_run = NULL;
      logger.msg(VERBOSE, "start_writing_ftp: thread create failed");
      buffer->error_write(true);
      writing = false;
      return DataStatus(DataStatus::WriteStartError, "Failed to create new thread");
    }
    return DataStatus::Success;
  }

  DataStatus DataPointDelegate::StopWriting() {
    if (!writing) return DataStatus::WriteStopError;
    writing = false;
    if(!helper_run) return DataStatus::Success; // never started? already stopped?
    // If error in buffer then write thread have already called abort
    if(buffer && !buffer->eof_write() && !buffer->error()) { // otherwise it will exit itself
      logger.msg(VERBOSE, "StopWriting: aborting connection");
      buffer->error_write(true);
    }
    helper_run->Kill(1);
    // Waiting for data transfer thread to finish
    cond.wait();
    helper_run = NULL;
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

  void DataPointDelegate::write_thread(void *arg) {
    DataPointDelegate *it = (DataPointDelegate*)arg;
    if(!it) return;
    Arc::CountedPointer<Run> run(it->helper_run);
    DataBuffer& buffer(*(it->buffer));
    bool out_failed = false;
    if(run) {
      int timeout = it->usercfg.Timeout()*1000;
      logger.msg(INFO, "write_thread: get and pass buffers");
      for (;;) {
        int h;
        unsigned int l;
        unsigned long long int o;
        if (!buffer.for_write(h, l, o, true)) {
          if (buffer.error()) {
            logger.msg(VERBOSE, "write_thread: for_write failed - aborting");
            buffer.error_write(true);
            break;
          }
          logger.msg(VERBOSE, "write_thread: for_write eof");
          // no buffers and no errors - must be pure eof
          o = buffer.eof_position();
          DataExternalComm::DataChunkExtBuffer dc;
          // No timeout here. Timeouts are implemented externally through call to StopWriting().
          if((!DataExternalComm::OutTag(*run, -1, DataExternalComm::DataChunkTag)) || (!dc.write(*run, -1, NULL, o, 0))) {
            out_failed = true;
            break;
          }
          break;
        }
        if(l > 0) {
          DataExternalComm::DataChunkExtBuffer dc;
          if((!DataExternalComm::OutTag(*run, -1, DataExternalComm::DataChunkTag)) || (!dc.write(*run, -1, buffer[h], o, l))) {
            logger.msg(VERBOSE, "write_thread: out failed - aborting");
            buffer.is_notwritten(h);
            out_failed = true;
            break;
          }
        }
        buffer.is_written(h);
      }
      logger.msg(VERBOSE, "write_thread: exiting");
      if(out_failed) {
        // Communication with helper failed but may still read status
        it->data_status = it->EndCommand(run, DataStatus::WriteError);
        buffer.error_write(true);
      } else if(buffer.error_write()) {
        // Report generic error
        it->data_status = DataStatus::WriteError;
      } else {
        // So far so good - read status
        it->data_status = it->EndCommand(run, DataStatus::WriteError);
        buffer.eof_write(true);
      }
    } else {
      it->data_status = DataStatus::WriteError;
    }
    it->cond.signal(); // Report to control thread that data transfer thread finished
  }

  DataStatus DataPointDelegate::Stat(FileInfo& file, DataPoint::DataPointInfoType verb) {
    if (reading) return DataStatus::IsReadingError;
    if (writing) return DataStatus::IsWritingError;

    std::list<std::string> argv(additional_args);
    argv.push_back(StatCommand);
    argv.push_back(url.fullstr());
    argv.push_back(Arc::tostring(verb));
    Arc::CountedPointer<Arc::Run> run;
    DataStatus result = StartCommand(run, argv, DataStatus::StatError);
    if(!result) return result;
    // Expecting one FileInfo
    bool file_is_available = false;
    char tag = DataExternalComm::InTag(*run, 1000*usercfg.Timeout());
    if(tag == DataExternalComm::FileInfoTag) { // Expected
      if(DataExternalComm::InEntry(*run, 1000*usercfg.Timeout(), file)) {
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

  DataStatus DataPointDelegate::List(std::list<FileInfo>& files, DataPoint::DataPointInfoType verb) {
    if (reading) return DataStatus::IsReadingError;
    if (writing) return DataStatus::IsWritingError;

    std::list<std::string> argv(additional_args);
    argv.push_back(ListCommand);
    argv.push_back(url.fullstr());
    argv.push_back(Arc::tostring(verb));
    Arc::CountedPointer<Arc::Run> run;
    DataStatus result = StartCommand(run, argv, DataStatus::ListError);
    if(!result) return result;
    // Expecting [0-inf) FileInfo
    char tag = DataExternalComm::InTag(*run, 1000*usercfg.Timeout());
    while(tag == DataExternalComm::FileInfoTag) {
      Arc::FileInfo file;
      if(!DataExternalComm::InEntry(*run, 1000*usercfg.Timeout(), file)) {
        // Communication is broken - can't proceed with reading
        result = DataStatus(DataStatus::ListError, "Failed to read helper result for "+url.plainstr());
        break;
      }
      file.SetName(file.GetLastName()); // relative name is expected historically
      files.push_back(file);
      tag = DataExternalComm::InTag(*run, 1000*usercfg.Timeout());
    }
    if(result) {
      result = EndCommand(run, DataStatus::ListError, tag);
    }
    if(!result) return result; 
    return DataStatus::Success;
  }

  DataStatus DataPointDelegate::Rename(const URL& newurl) {
    if (reading) return DataStatus::IsReadingError;
    if (writing) return DataStatus::IsWritingError;

    std::list<std::string> argv(additional_args);
    argv.push_back(RenameCommand);
    argv.push_back(url.fullstr());
    argv.push_back(newurl.fullstr());
    Arc::CountedPointer<Arc::Run> run;
    DataStatus result = StartCommand(run, argv, DataStatus::RenameError);
    if(!result) return result;
    result = EndCommand(run, DataStatus::RenameError);
    if(!result) return result; 
    return DataStatus::Success;
  }

  DataStatus DataPointDelegate::Transfer(const URL& otherendpoint, bool source, TransferCallback callback) {
    if(!SupportsTransfer())
      return DataStatus(DataStatus::UnimplementedError, EOPNOTSUPP);
    if (reading) return DataStatus::IsReadingError;
    if (writing) return DataStatus::IsWritingError;

    std::list<std::string> argv(additional_args);
    if (source) {
      argv.push_back(TransferFromCommand);
    } else {
      argv.push_back(TransferToCommand);
    }
    argv.push_back(url.fullstr());
    argv.push_back(otherendpoint.fullstr());
    Arc::CountedPointer<Arc::Run> run;
    DataStatus result = StartCommand(run, argv, DataStatus::TransferError);
    if(!result) return result;
    // Read callback information till end tag is received
    // Looks like Transfer() method was designed to not timeout.
    // Hence waiting for each tag indefinitely.
    char tag = DataExternalComm::InTag(*run, -1);
    while(tag == DataExternalComm::TransferStatusTag) {
      DataExternalComm::TransferStatus transfer_status(0);
      if(!DataExternalComm::InEntry(*run, 1000*usercfg.Timeout(), transfer_status)) {
        return DataStatus(DataStatus::TransferError, "Failed to read data transfer status from helper process for "+url.plainstr());
      }
      if(callback) (*callback)(transfer_status.bytes_count); 
      tag = DataExternalComm::InTag(*run, -1);
    }
    result = EndCommand(run, DataStatus::TransferError, tag);
    if(!result) return result; 
    return DataStatus::Success;
  }

  DataStatus DataPointDelegate::Transfer3rdParty(const URL& source, const URL& destination, TransferCallback callback) {
    std::list<std::string> argv(additional_args);
    argv.push_back(Transfer3rdCommand);
    argv.push_back(source.fullstr());
    argv.push_back(destination.fullstr());
    Arc::CountedPointer<Arc::Run> run;
    DataStatus result = StartCommand(run, argv, DataStatus::TransferError);
    if(!result) return result;
    // Read callback information till end tag is received
    // Looks like Transfer3rdParty() method was designed to not timeout.
    // Hence waiting for each tag indefinitely.
    char tag = DataExternalComm::InTag(*run, -1);
    while(tag == DataExternalComm::TransferStatusTag) {
      DataExternalComm::TransferStatus transfer_status(0);
      if(!DataExternalComm::InEntry(*run, 1000*usercfg.Timeout(), transfer_status)) {
        return DataStatus(DataStatus::TransferError, "Failed to read data transfer status from helper process for "+url.plainstr());
      }
      if(callback) (*callback)(transfer_status.bytes_count);
      tag = DataExternalComm::InTag(*run, -1);
    }
    result = EndCommand(run, DataStatus::TransferError, tag);
    if(!result) return result;
    return DataStatus::Success;
  }

  DataPointDelegate::DataPointDelegate(char const* module_name, const URL& url, const UserConfig& usercfg, PluginArgument* parg) :
    DataPointDirect(url, usercfg, parg),
      reading(false),
      writing(false),
      helper_run(),
      exec_path(Arc::ArcLocation::GetLibDir()+G_DIR_SEPARATOR_S+"arc-dmc") {
    additional_args.push_back(Arc::ArcLocation::GetLibDir()+G_DIR_SEPARATOR_S+"external"+G_DIR_SEPARATOR_S);
    additional_args.push_back(module_name?module_name:"");
  }

  DataPointDelegate::DataPointDelegate(char const* exec_path, std::list<std::string> const & extra_args, const URL& url, const UserConfig& usercfg, PluginArgument* parg) :
    DataPointDirect(url, usercfg, parg),
      reading(false),
      writing(false),
      helper_run(),
      exec_path(exec_path?exec_path:"") {
  }

  DataPointDelegate::~DataPointDelegate() {
    StopReading();
    StopWriting();
  }

  bool DataPointDelegate::WriteOutOfOrder() {
    // implement
    return true;
  }

  bool DataPointDelegate::ProvidesMeta() const {
    // implement
    return true;
  }

  const std::string DataPointDelegate::DefaultCheckSum() const {
    // no way to know which checksum is used for each file, so hard-code adler32 for now
    // implement
    return std::string("adler32");
  }

  bool DataPointDelegate::RequiresCredentials() const {
    // implement
    return false;
  }

  std::string::size_type const DataPointDelegate::LogRedirect::level_size_max_ = 32;
  std::string::size_type const DataPointDelegate::LogRedirect::buffer_size_max_ = 4096;

  void DataPointDelegate::LogRedirect::Append(char const* data, unsigned int size) {
    while(true) {
      char const* sep = (char const*)memchr(data, '\n', size);
      if (sep == NULL) break;
      if(buffer_.length() < buffer_size_max_) buffer_.append(data,sep-data);
      Flush();
      size -= sep-data+1;
      data = sep+1;
    }
    if (size > 0) buffer_.append(data,size);
  }

  void DataPointDelegate::LogRedirect::Flush() {
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

} // namespace Arc

