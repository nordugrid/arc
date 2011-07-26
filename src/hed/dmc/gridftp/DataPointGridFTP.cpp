// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <openssl/ssl.h>

#include <arc/Logger.h>
#include <arc/StringConv.h>
#include <arc/UserConfig.h>
#include <arc/data/DataBuffer.h>
#include <arc/data/CheckSum.h>
#include <arc/globusutils/GlobusErrorUtils.h>
#include <arc/globusutils/GlobusWorkarounds.h>
#include <arc/globusutils/GSSCredential.h>
#include <arc/crypto/OpenSSL.h>

#include "DataPointGridFTP.h"
#include "Lister.h"

namespace Arc {

  static bool proxy_initialized = false;

  Logger DataPointGridFTP::logger(Logger::getRootLogger(), "DataPoint.GridFTP");

  void DataPointGridFTP::ftp_complete_callback(void *arg,
                                               globus_ftp_client_handle_t*,
                                               globus_object_t *error) {
    DataPointGridFTP *it = (DataPointGridFTP*)arg;
    if (error == GLOBUS_SUCCESS) {
      logger.msg(DEBUG, "ftp_complete_callback: success");
      it->cond.signal();
    }
    else {
      logger.msg(VERBOSE, "ftp_complete_callback: error: %s", globus_object_to_string(error));
      it->callback_error = trim(globus_object_to_string(error));
      it->cond.signal();
    }
  }

  void DataPointGridFTP::ftp_check_callback(void *arg,
                                            globus_ftp_client_handle_t*,
                                            globus_object_t *error,
                                            globus_byte_t*,
                                            globus_size_t length,
                                            globus_off_t,
                                            globus_bool_t eof) {
    logger.msg(VERBOSE, "ftp_check_callback");
    DataPointGridFTP *it = (DataPointGridFTP*)arg;
    if (error != GLOBUS_SUCCESS) {
      logger.msg(VERBOSE, "Globus error: %s", globus_object_to_string(error));
      return;
    }
    if (eof) {
      it->ftp_eof_flag = true;
      return;
    }
    if (it->check_received_length > 0) {
      logger.msg(INFO,
                 "Excessive data received while checking file access");
      it->ftp_eof_flag = true;
      globus_ftp_client_abort(&(it->ftp_handle));
      return;
    }
    it->check_received_length += length;
    GlobusResult res =
      globus_ftp_client_register_read(&(it->ftp_handle),
                                      (globus_byte_t*)(it->ftp_buf),
                                      sizeof(it->ftp_buf),
                                      &ftp_check_callback, it);
    if (!res) {
      logger.msg(INFO,
                 "Registration of Globus FTP buffer failed - cancel check");
      logger.msg(VERBOSE, "Globus error: %s", res.str());
      globus_ftp_client_abort(&(it->ftp_handle));
      return;
    }
    return;
  }

  DataStatus DataPointGridFTP::Check() {
    if (!ftp_active)
      return DataStatus::NotInitializedError;
    if (reading)
      return DataStatus::IsReadingError;
    if (writing)
      return DataStatus::IsWritingError;
    GlobusResult res;
    globus_off_t size = 0;
    globus_abstime_t gl_modify_time;
    time_t modify_time;
    set_attributes();
    res = globus_ftp_client_size(&ftp_handle, url.str().c_str(), &ftp_opattr,
                                 &size, &ftp_complete_callback, this);
    if (!res) {
      logger.msg(VERBOSE, "check_ftp: globus_ftp_client_size failed");
      logger.msg(INFO, "Globus error: %s", res.str());
    }
    else if (!cond.wait(1000*usercfg.Timeout())) {
      logger.msg(INFO, "check_ftp: timeout waiting for size");
      globus_ftp_client_abort(&ftp_handle);
      cond.wait();
    }
    else if (!callback_error.empty())
      logger.msg(INFO, "check_ftp: failed to get file's size");
    else {
      SetSize(size);
      logger.msg(VERBOSE, "check_ftp: obtained size: %lli", GetSize());
    }
    res = globus_ftp_client_modification_time(&ftp_handle, url.str().c_str(),
                                              &ftp_opattr, &gl_modify_time,
                                              &ftp_complete_callback, this);
    if (!res) {
      logger.msg(VERBOSE,
                 "check_ftp: globus_ftp_client_modification_time failed");
      logger.msg(INFO, "Globus error: %s", res.str());
    }
    else if (!cond.wait(1000*usercfg.Timeout())) {
      logger.msg(INFO, "check_ftp: timeout waiting for modification_time");
      globus_ftp_client_abort(&ftp_handle);
      cond.wait();
    }
    else if (!callback_error.empty())
      logger.msg(INFO, "check_ftp: failed to get file's modification time");
    else {
      int modify_utime;
      GlobusTimeAbstimeGet(gl_modify_time, modify_time, modify_utime);
      SetCreated(modify_time);
      logger.msg(VERBOSE, "check_ftp: obtained creation date: %s", GetCreated().str());
    }
    // check if file or directory - can't do a get on a directory
    FileInfo fileinfo;
    if (!Stat(fileinfo, INFO_TYPE_TYPE))
      return DataStatus::CheckError;
    if (fileinfo.GetType() != FileInfo::file_type_file)
      // successful stat is enough to report successful access to a directory
      return DataStatus::Success;

    // Do not use partial_get for ordinary ftp. Stupid globus tries to
    // use non-standard commands anyway.
    if (is_secure) {
      res = globus_ftp_client_partial_get(&ftp_handle, url.str().c_str(),
                                          &ftp_opattr, GLOBUS_NULL, 0, 1,
                                          &ftp_complete_callback, this);
      if (!res) {
        logger.msg(VERBOSE, "check_ftp: globus_ftp_client_get failed");
        logger.msg(ERROR, res.str());
        return DataStatus::CheckError;
      }
      // use eof_flag to pass result from callback
      ftp_eof_flag = false;
      check_received_length = 0;
      logger.msg(VERBOSE, "check_ftp: globus_ftp_client_register_read");
      res = globus_ftp_client_register_read(&ftp_handle,
                                            (globus_byte_t*)ftp_buf,
                                            sizeof(ftp_buf),
                                            &ftp_check_callback, this);
      if (!res) {
        globus_ftp_client_abort(&ftp_handle);
        cond.wait();
        return DataStatus::CheckError;
      }
      if (!cond.wait(1000*usercfg.Timeout())) {
        logger.msg(ERROR, "check_ftp: timeout waiting for partial get");
        globus_ftp_client_abort(&ftp_handle);
        cond.wait();
        return DataStatus::CheckError;
      }
      if (ftp_eof_flag || callback_error.empty()) return DataStatus::Success;
      return DataStatus(DataStatus::CheckError, callback_error);
    }
    else {
      // Do not use it at all. It does not give too much useful
      // information anyway. But request at least existence of file.
      if (!CheckSize())
        return DataStatus::CheckError;
      return DataStatus::Success;
    }
  }

  DataStatus DataPointGridFTP::Remove() {
    if (!ftp_active)
      return DataStatus::NotInitializedError;
    if (reading)
      return DataStatus::IsReadingError;
    if (writing)
      return DataStatus::IsWritingError;
    GlobusResult res;
    set_attributes();
    res = globus_ftp_client_delete(&ftp_handle, url.str().c_str(),
                                   &ftp_opattr, &ftp_complete_callback, this);
    if (!res) {
      logger.msg(VERBOSE, "delete_ftp: globus_ftp_client_delete failed");
      std::string globus_err(res.str());
      logger.msg(ERROR, globus_err);
      return DataStatus(DataStatus::DeleteError, globus_err);
    }
    if (!cond.wait(1000*usercfg.Timeout())) {
      logger.msg(ERROR, "delete_ftp: timeout waiting for delete");
      globus_ftp_client_abort(&ftp_handle);
      cond.wait();
      return DataStatus(DataStatus::DeleteError, "Timeout waiting for delete");
    }
    if (!callback_error.empty()) {
      logger.msg(ERROR, callback_error);
      return DataStatus(DataStatus::DeleteError, callback_error);
    }
    return DataStatus::Success;
  }

  static bool remove_last_dir(std::string& dir) {
    // dir also contains proto and server
    std::string::size_type nn = std::string::npos;
    if (!strncasecmp(dir.c_str(), "ftp://", 6))
      nn = dir.find('/', 6);
    else if (!strncasecmp(dir.c_str(), "gsiftp://", 9))
      nn = dir.find('/', 9);
    if (nn == std::string::npos)
      return false;
    std::string::size_type n;
    if ((n = dir.rfind('/')) == std::string::npos)
      return false;
    if (n < nn)
      return false;
    dir.resize(n);
    return true;
  }

  static bool add_last_dir(std::string& dir, const std::string& path) {
    int l = dir.length();
    std::string::size_type n = path.find('/', l + 1);
    if (n == std::string::npos)
      return false;
    dir = path;
    dir.resize(n);
    return true;
  }

  bool DataPointGridFTP::mkdir_ftp() {
    ftp_dir_path = url.str();
    for (;;)
      if (!remove_last_dir(ftp_dir_path))
        break;
    bool result = false;
    for (;;) {
      if (!add_last_dir(ftp_dir_path, url.str()))
        break;
      logger.msg(VERBOSE, "mkdir_ftp: making %s", ftp_dir_path);
      GlobusResult res =
        globus_ftp_client_mkdir(&ftp_handle, ftp_dir_path.c_str(), &ftp_opattr,
                                &ftp_complete_callback, this);
      if (!res) {
        logger.msg(INFO, "Globus error: %s", res.str());
        return false;
      }
      if (!cond.wait(1000*usercfg.Timeout())) {
        logger.msg(INFO, "mkdir_ftp: timeout waiting for mkdir");
        /* timeout - have to cancel operation here */
        globus_ftp_client_abort(&ftp_handle);
        cond.wait();
        return false;
      }
      if (!callback_error.empty())
        result = false;
    }
    return result;
  }

  DataStatus DataPointGridFTP::StartReading(DataBuffer& buf) {
    if (!ftp_active)
      return DataStatus::NotInitializedError;
    if (reading)
      return DataStatus::IsReadingError;
    if (writing)
      return DataStatus::IsWritingError;
    set_attributes();
    reading = true;
    buffer = &buf;
    bool limit_length = false;
    unsigned long long int range_length = 0;
    if (range_end > range_start) {
      range_length = range_end - range_start;
      limit_length = true;
    }
    logger.msg(VERBOSE, "start_reading_ftp");
    ftp_eof_flag = false;
    globus_ftp_client_handle_cache_url_state(&ftp_handle, url.str().c_str());
    GlobusResult res;
    logger.msg(VERBOSE, "start_reading_ftp: globus_ftp_client_get");
    if (limit_length)
      res = globus_ftp_client_partial_get(&ftp_handle, url.str().c_str(),
                                          &ftp_opattr, GLOBUS_NULL,
                                          range_start,
                                          range_start + range_length + 1,
                                          &ftp_get_complete_callback, this);
    else
      res = globus_ftp_client_get(&ftp_handle, url.str().c_str(),
                                  &ftp_opattr, GLOBUS_NULL,
                                  &ftp_get_complete_callback, this);
    if (!res) {
      logger.msg(VERBOSE, "start_reading_ftp: globus_ftp_client_get failed");
      logger.msg(ERROR, res.str());

      globus_ftp_client_handle_flush_url_state(&ftp_handle, url.str().c_str());
      buffer->error_read(true);
      reading = false;
      return DataStatus::ReadStartError;
    }
    if (globus_thread_create(&ftp_control_thread, GLOBUS_NULL,
                             &ftp_read_thread, this) != 0) {
      logger.msg(VERBOSE, "start_reading_ftp: globus_thread_create failed");
      globus_ftp_client_abort(&ftp_handle);
      cond.wait();
      globus_ftp_client_handle_flush_url_state(&ftp_handle, url.str().c_str());
      buffer->error_read(true);
      reading = false;
      return DataStatus::ReadStartError;
    }
    // make sure globus has thread for handling network/callbacks
    globus_thread_blocking_will_block();
    return DataStatus::Success;
  }

  DataStatus DataPointGridFTP::StopReading() {
    if (!reading)
      return DataStatus::ReadStopError;
    reading = false;
    if (!buffer->eof_read()) {
      logger.msg(VERBOSE, "stop_reading_ftp: aborting connection");
      globus_ftp_client_abort(&ftp_handle);
    }
    logger.msg(VERBOSE, "stop_reading_ftp: waiting for transfer to finish");
    cond.wait();
    logger.msg(VERBOSE, "stop_reading_ftp: exiting: %s", url.str());
    //globus_ftp_client_handle_flush_url_state(&ftp_handle, url.str().c_str());
    if (!callback_error.empty()) return DataStatus(DataStatus::ReadStopError, callback_error);
    return DataStatus::Success;
  }

  void* DataPointGridFTP::ftp_read_thread(void *arg) {
    DataPointGridFTP *it = (DataPointGridFTP*)arg;
    int h;
    unsigned int l;
    GlobusResult res;
    int registration_failed = 0;
    logger.msg(INFO, "ftp_read_thread: get and register buffers");
    int n_buffers = 0;
    for (;;) {
      if (it->buffer->eof_read())
        break;
      if (!it->buffer->for_read(h, l, true)) { /* eof or error */
        if (it->buffer->error()) { /* error -> abort reading */
          logger.msg(VERBOSE, "ftp_read_thread: for_read failed - aborting: %s",
                     it->url.str());
          globus_ftp_client_abort(&(it->ftp_handle));
        }
        break;
      }
      res =
        globus_ftp_client_register_read(&(it->ftp_handle),
                                        (globus_byte_t*)((*(it->buffer))[h]),
                                        l, &(it->ftp_read_callback), it);
      if (!res) {
        logger.msg(DEBUG, "ftp_read_thread: Globus error: %s", res.str());
        // This can happen if handle can't either yet or already 
        // provide data. In last case there is no reason to retry.
        if(it->ftp_eof_flag) {
          it->buffer->is_read(h, 0, 0);
          break;
        }
        registration_failed++;
        if (registration_failed >= 10) {
          it->buffer->is_read(h, 0, 0);
          it->buffer->error_read(true);
          // can set eof here because no callback will be called (I guess).
          it->buffer->eof_read(true);
          logger.msg(DEBUG, "ftp_read_thread: "
                     "too many registration failures - abort: %s",
                     it->url.str());
        }
        else {
          logger.msg(DEBUG, "ftp_read_thread: "
                     "failed to register globus buffer - will try later: %s",
                     it->url.str());
          it->buffer->is_read(h, 0, 0);
          // First retry quickly for race condition.
          // Then slowly for pecularities.
          if(registration_failed > 2) sleep(1);
        }
      }
      else
        n_buffers++;
    }
    // make sure complete callback is called
    logger.msg(VERBOSE, "ftp_read_thread: waiting for eof");
    it->buffer->wait_eof_read();
    // And now make sure all buffers were released in case Globus calls
    // complete_callback before calling all read_callbacks
    logger.msg(VERBOSE, "ftp_read_thread: waiting for buffers released");
    it->buffer->wait_for_read();
    logger.msg(VERBOSE, "ftp_read_thread: exiting");
    if (it->buffer->error_read()) it->callback_error = "Read error";
    it->cond.signal();
    return NULL;
  }

  void DataPointGridFTP::ftp_read_callback(void *arg,
                                           globus_ftp_client_handle_t*,
                                           globus_object_t *error,
                                           globus_byte_t *buffer,
                                           globus_size_t length,
                                           globus_off_t offset,
                                           globus_bool_t eof) {
    DataPointGridFTP *it = (DataPointGridFTP*)arg;
    if (error != GLOBUS_SUCCESS) {
      logger.msg(VERBOSE, "ftp_read_callback: failure: %s",globus_object_to_string(error));
      it->buffer->is_read((char*)buffer, 0, 0);
      return;
    }
    logger.msg(DEBUG, "ftp_read_callback: success");
    it->buffer->is_read((char*)buffer, length, offset);
    if (eof)
      it->ftp_eof_flag = true;
    return;
  }

  void DataPointGridFTP::ftp_get_complete_callback(void *arg,
                                                   globus_ftp_client_handle_t*,
                                                   globus_object_t *error) {
    DataPointGridFTP *it = (DataPointGridFTP*)arg;
    /* data transfer finished */
    if (error != GLOBUS_SUCCESS) {
      logger.msg(INFO, "Failed to get ftp file");
      logger.msg(ERROR, trim(globus_object_to_string(error)));
      it->failure_code = DataStatus(DataStatus::ReadStartError, globus_object_to_string(error));
      it->buffer->error_read(true);
      return;
    }
    it->buffer->eof_read(true);
    return;
  }

  DataStatus DataPointGridFTP::StartWriting(DataBuffer& buf,
                                            DataCallback*) {
    if (!ftp_active)
      return DataStatus::NotInitializedError;
    if (reading)
      return DataStatus::IsReadingError;
    if (writing)
      return DataStatus::IsWritingError;
    set_attributes();
    writing = true;
    buffer = &buf;
    /* size of file first */
    bool limit_length = false;
    unsigned long long int range_length = 0;
    if (range_end > range_start) {
      range_length = range_end - range_start;
      limit_length = true;
    }
    ftp_eof_flag = false;
    GlobusResult res;
    globus_ftp_client_handle_cache_url_state(&ftp_handle, url.str().c_str());
    if (autodir) {
      logger.msg(VERBOSE, "start_writing_ftp: mkdir");
      if (!mkdir_ftp())
        logger.msg(VERBOSE,
                   "start_writing_ftp: mkdir failed - still trying to write");
    }
    logger.msg(VERBOSE, "start_writing_ftp: put");
    if (limit_length)
      res = globus_ftp_client_partial_put(&ftp_handle, url.str().c_str(),
                                          &ftp_opattr, GLOBUS_NULL,
                                          range_start,
                                          range_start + range_length,
                                          &ftp_put_complete_callback, this);
    else
      res = globus_ftp_client_put(&ftp_handle, url.str().c_str(),
                                  &ftp_opattr, GLOBUS_NULL,
                                  &ftp_put_complete_callback, this);
    if (!res) {
      logger.msg(VERBOSE, "start_writing_ftp: put failed");
      logger.msg(ERROR, res.str());
      globus_ftp_client_handle_flush_url_state(&ftp_handle, url.str().c_str());
      buffer->error_write(true);
      writing = false;
      return DataStatus::WriteStartError;
    }
    if (globus_thread_create(&ftp_control_thread, GLOBUS_NULL,
                             &ftp_write_thread, this) != 0) {
      logger.msg(VERBOSE, "start_writing_ftp: globus_thread_create failed");
      globus_ftp_client_handle_flush_url_state(&ftp_handle, url.str().c_str());
      buffer->error_write(true);
      writing = false;
      return DataStatus::WriteStartError;
    }
    // make sure globus has thread for handling network/callbacks
    globus_thread_blocking_will_block();
    return DataStatus::Success;
  }

  DataStatus DataPointGridFTP::StopWriting() {
    if (!writing)
      return DataStatus::WriteStopError;
    writing = false;
    if (!buffer->eof_write()) {
      logger.msg(VERBOSE, "StopWriting: aborting connection");
      globus_ftp_client_abort(&ftp_handle);
    }
    cond.wait();
    // checksum verification
    const CheckSum * calc_sum = buffer->checksum_object();
    if (calc_sum && *calc_sum && buffer->checksum_valid()) {
      char buf[100];
      calc_sum->print(buf,100);
      std::string csum(buf);
      if (csum.substr(0, csum.find(':')) == DefaultCheckSum()) {
        logger.msg(VERBOSE, "StopWriting: Calculated checksum %s", csum);
        // list checksum and compare
        // note: not all implementations support checksum
        logger.msg(DEBUG, "list_files_ftp: "
                          "looking for checksum of %s", url.str());
        char cksum[256];
        std::string cksumtype(upper(DefaultCheckSum()));
        GlobusResult res = globus_ftp_client_cksm(&ftp_handle, url.str().c_str(),
                                                  &ftp_opattr, cksum, (globus_off_t)0,
                                                  (globus_off_t)-1, cksumtype.c_str(),
                                                  &ftp_complete_callback, this);
        if (!res) {
          logger.msg(VERBOSE, "list_files_ftp: globus_ftp_client_cksum failed");
          logger.msg(VERBOSE, "Globus error: %s", res.str());
        }
        else if (!cond.wait(1000*usercfg.Timeout())) {
          logger.msg(VERBOSE, "list_files_ftp: timeout waiting for cksum");
          globus_ftp_client_abort(&ftp_handle);
          cond.wait();
        }
        else if (!callback_error.empty()) {
          // reset to success since failing to get checksum should not trigger an error
          callback_error.clear();
          logger.msg(INFO, "list_files_ftp: failed to get file's checksum");
        }
        else {
          logger.msg(VERBOSE, "list_files_ftp: checksum %s", cksum);
          if (csum.substr(csum.find(':')+1) == std::string(cksum)) {
            logger.msg(INFO, "Calculated checksum %s matches checksum reported by server", csum);
            SetCheckSum(csum);
          } else {
            logger.msg(ERROR, "Checksum mismatch between calculated checksum %s and checksum reported by server %s",
                       csum, std::string(DefaultCheckSum()+':'+cksum));
            return DataStatus::TransferErrorRetryable;
          }
        }
      }
    }
    //globus_ftp_client_handle_flush_url_state(&ftp_handle, url.str().c_str());
    if (!callback_error.empty()) return DataStatus(DataStatus::WriteStopError, callback_error);
    return DataStatus::Success;
  }

  void* DataPointGridFTP::ftp_write_thread(void *arg) {
    DataPointGridFTP *it = (DataPointGridFTP*)arg;
    int h;
    unsigned int l;
    unsigned long long int o;
    GlobusResult res;
    globus_bool_t eof = GLOBUS_FALSE;
    logger.msg(INFO, "ftp_write_thread: get and register buffers");
    for (;;) {
      if (!it->buffer->for_write(h, l, o, true)) {
        if (it->buffer->error()) {
          logger.msg(VERBOSE, "ftp_write_thread: for_write failed - aborting");
          globus_ftp_client_abort(&(it->ftp_handle));
          break;
        }
        // no buffers and no errors - must be pure eof
        eof = GLOBUS_TRUE;
        char dummy;
        o = it->buffer->eof_position();
        res = globus_ftp_client_register_write(&(it->ftp_handle),
                                               (globus_byte_t*)(&dummy), 0, o,
                                               eof, &ftp_write_callback, it);
        break;
        // if(res == GLOBUS_SUCCESS) break;
        // sleep(1); continue;
      }
      res =
        globus_ftp_client_register_write(&(it->ftp_handle),
                                         (globus_byte_t*)((*(it->buffer))[h]),
                                         l, o, eof, &ftp_write_callback, it);
      if (!res) {
        it->buffer->is_notwritten(h);
        sleep(1);
      }
    }
    // make sure complete callback is called
    logger.msg(VERBOSE, "ftp_write_thread: waiting for eof");
    it->buffer->wait_eof_write();
    // And now make sure all buffers were released in case Globus calls
    // complete_callback before calling all read_callbacks
    logger.msg(VERBOSE, "ftp_write_thread: waiting for buffers released");
    it->buffer->wait_for_write();
    if (it->buffer->error_write()) it->callback_error = "Write error";
    it->cond.signal();
    return NULL;
  }

  void DataPointGridFTP::ftp_write_callback(void *arg,
                                            globus_ftp_client_handle_t*,
                                            globus_object_t *error,
                                            globus_byte_t *buffer,
                                            globus_size_t,
                                            globus_off_t,
                                            globus_bool_t) {
    DataPointGridFTP *it = (DataPointGridFTP*)arg;
    if (error != GLOBUS_SUCCESS) {
      logger.msg(VERBOSE, "ftp_write_callback: failure: %s",globus_object_to_string(error));
      it->buffer->is_written((char*)buffer);
      return;
    }
    logger.msg(DEBUG, "ftp_write_callback: success");
    it->buffer->is_written((char*)buffer);
    return;
  }

  void DataPointGridFTP::ftp_put_complete_callback(void *arg,
                                                   globus_ftp_client_handle_t*,
                                                   globus_object_t *error) {
    DataPointGridFTP *it = (DataPointGridFTP*)arg;
    /* data transfer finished */
    if (error != GLOBUS_SUCCESS) {
      logger.msg(INFO, "Failed to store ftp file");
      it->failure_code = DataStatus(DataStatus::WriteStartError, globus_object_to_string(error));
      logger.msg(ERROR, trim(globus_object_to_string(error)));
      it->buffer->error_write(true);
      return;
    }
    it->buffer->eof_write(true);
    return;
  }

  DataStatus DataPointGridFTP::do_more_stat(FileInfo& f) {
    DataStatus result = DataStatus::Success;
    GlobusResult res;
    globus_off_t size = 0;
    globus_abstime_t gl_modify_time;
    time_t modify_time;
    std::string f_url = url.ConnectionURL() + f.GetName();
    if ((!f.CheckSize()) && (f.GetType() != FileInfo::file_type_dir)) {
      logger.msg(DEBUG, "list_files_ftp: looking for size of %s", f_url);
      res = globus_ftp_client_size(&ftp_handle, f_url.c_str(), &ftp_opattr,
                                   &size, &ftp_complete_callback, this);
      if (!res) {
        logger.msg(VERBOSE, "list_files_ftp: globus_ftp_client_size failed");
        std::string globus_err(res.str());
        logger.msg(INFO, "Globus error: %s", globus_err);
        result = DataStatus(DataStatus::StatError, globus_err);
      }
      else if (!cond.wait(1000*usercfg.Timeout())) {
        logger.msg(INFO, "list_files_ftp: timeout waiting for size");
        logger.msg(INFO, "list_files_ftp: timeout waiting for size");
        globus_ftp_client_abort(&ftp_handle);
        cond.wait();
        result = DataStatus(DataStatus::StatError, "timeout waiting for file size");
      }
      else if (!callback_error.empty()) {
        logger.msg(INFO, "list_files_ftp: failed to get file's size");
        result = DataStatus(DataStatus::StatError, callback_error);
        // Guessing - directories usually have no size
        f.SetType(FileInfo::file_type_dir);
      }
      else {
        f.SetSize(size);
        // Guessing - only files usually have size
        f.SetType(FileInfo::file_type_file);
      }
    }
    if (!f.CheckCreated()) {
      logger.msg(DEBUG, "list_files_ftp: "
                        "looking for modification time of %s", f_url);
      res = globus_ftp_client_modification_time(&ftp_handle, f_url.c_str(),
                                                &ftp_opattr, &gl_modify_time,
                                                &ftp_complete_callback, this);
      if (!res) {
        logger.msg(VERBOSE, "list_files_ftp: "
                            "globus_ftp_client_modification_time failed");
        std::string globus_err(res.str());
        logger.msg(INFO, "Globus error: %s", globus_err);
        result = DataStatus(DataStatus::StatError, globus_err);
      }
      else if (!cond.wait(1000*usercfg.Timeout())) {
        logger.msg(INFO, "list_files_ftp: "
                         "timeout waiting for modification_time");
        globus_ftp_client_abort(&ftp_handle);
        cond.wait();
        result = DataStatus(DataStatus::StatError, "timeout waiting for file size");
      }
      else if (!callback_error.empty()) {
        logger.msg(INFO, "list_files_ftp: "
                         "failed to get file's modification time");
        result = DataStatus(DataStatus::StatError, callback_error);
      }
      else {
        int modify_utime;
        GlobusTimeAbstimeGet(gl_modify_time, modify_time, modify_utime);
        f.SetCreated(modify_time);
      }
    }
    if (!f.CheckCheckSum() && f.GetType() != FileInfo::file_type_dir) {
      // not all implementations support checksum so failure is not an error
      logger.msg(DEBUG, "list_files_ftp: "
                        "looking for checksum of %s", f_url);
      char cksum[256];
      std::string cksumtype(upper(DefaultCheckSum()).c_str());
      res = globus_ftp_client_cksm(&ftp_handle, f_url.c_str(),
                                   &ftp_opattr, cksum, (globus_off_t)0,
                                   (globus_off_t)-1, cksumtype.c_str(),
                                   &ftp_complete_callback, this);
      if (!res) {
        logger.msg(VERBOSE, "list_files_ftp: globus_ftp_client_cksum failed");
        logger.msg(VERBOSE, "Globus error: %s", res.str());
      }
      else if (!cond.wait(1000*usercfg.Timeout())) {
        logger.msg(VERBOSE, "list_files_ftp: timeout waiting for cksum");
        globus_ftp_client_abort(&ftp_handle);
        cond.wait();
      }
      else if (!callback_error.empty()) {
        logger.msg(INFO, "list_files_ftp: failed to get file's checksum");
      }
      else {
        logger.msg(VERBOSE, "list_files_ftp: checksum %s", cksum);
        f.SetCheckSum(DefaultCheckSum() + ':' + std::string(cksum));
      }
    }
    return result;
  }

  DataStatus DataPointGridFTP::Stat(FileInfo& file, DataPoint::DataPointInfoType verb) {
    if (!ftp_active)
      return DataStatus::NotInitializedError;
    if (reading)
      return DataStatus::IsReadingError;
    if (writing)
      return DataStatus::IsWritingError;
    set_attributes();
    Lister lister(*credential);
    bool more_info = ((verb | INFO_TYPE_NAME) != INFO_TYPE_NAME);
    DataStatus lister_res = lister.retrieve_file_info(url,!more_info);
    if (!lister_res) {
      logger.msg(ERROR, "Failed to obtain stat from ftp: %s", lister_res.GetDesc());
      return lister_res;
    }
    lister.close_connection();
    DataStatus result = DataStatus::StatError;
    if (lister.size() == 0) {
      logger.msg(ERROR, "No results returned from stat");
      result.SetDesc("No results found");
      return result;
    }
    if(lister.size() != 1) {
      logger.msg(VERBOSE, "Wrong number of objects (%i) for stat from ftp: %s", lister.size(), url.str());
      // guess - that probably means it is directory 
      file.SetName(FileInfo(url.Path()).GetName());
      file.SetType(FileInfo::file_type_dir);
      return DataStatus::Success;
    }
    FileInfo lister_info(*(lister.begin()));
    // does returned path match what we expect?
    // remove trailing slashes from url
    std::string fname(url.Path());
    while (fname.length() > 1 && fname[fname.length()-1] == '/') fname.erase(fname.length()-1);
    if ((lister_info.GetName().substr(lister_info.GetName().rfind('/')+1)) !=
              (fname.substr(fname.rfind('/')+1))) {
      logger.msg(ERROR, "Unexpected path %s returned from server", lister_info.GetName());
      result.SetDesc("Unexpected path returned from server");
      return result;
    }
    result = DataStatus::Success;
    if (lister_info.GetName()[0] != '/')
      lister_info.SetName(url.Path());

    file.SetName(lister_info.GetName());
    file.SetMetaData("path", lister_info.GetName());
    if (more_info) {
      DataStatus r = do_more_stat(lister_info);
      if(!r) result = r;
    }
    file.SetType(lister_info.GetType());
    file.SetMetaData("type", (lister_info.GetType() == FileInfo::file_type_dir) ? "dir" : "file");
    if (lister_info.CheckSize()) {
      file.SetSize(lister_info.GetSize());
      file.SetMetaData("size", tostring(lister_info.GetSize()));
      SetSize(lister_info.GetSize());
    }
    if (lister_info.CheckCreated()) {
      file.SetCreated(lister_info.GetCreated());
      file.SetMetaData("mtime", lister_info.GetCreated());
      SetCreated(lister_info.GetCreated());
    }
    if (lister_info.CheckCheckSum()) {
      file.SetCheckSum(lister_info.GetCheckSum());
      file.SetMetaData("checksum", lister_info.GetCheckSum());
      SetCheckSum(lister_info.GetCheckSum());
    }
    return result;
  }

  DataStatus DataPointGridFTP::List(std::list<FileInfo>& files, DataPoint::DataPointInfoType verb) {
    if (!ftp_active)
      return DataStatus::NotInitializedError;
    if (reading)
      return DataStatus::IsReadingError;
    if (writing)
      return DataStatus::IsWritingError;
    set_attributes();
    Lister lister(*credential);
    bool more_info = ((verb | INFO_TYPE_NAME) != INFO_TYPE_NAME);
    DataStatus lister_res = lister.retrieve_dir_info(url,!more_info);
    if (!lister_res) {
      logger.msg(ERROR, "Failed to obtain listing from ftp: %s", lister_res.GetDesc());
      return lister_res;
    }
    lister.close_connection();
    DataStatus result = DataStatus::Success;
    for (std::list<FileInfo>::iterator i = lister.begin();
         i != lister.end(); ++i) {
      if (i->GetName()[0] != '/')
        i->SetName(url.Path()+'/'+i->GetName());
      std::list<FileInfo>::iterator f =
        files.insert(files.end(), FileInfo(i->GetLastName()));
      f->SetMetaData("path", i->GetLastName());
      if (more_info) {
        DataStatus r = do_more_stat(*i);
        if(!r) {
          if(r == DataStatus::StatError) r = DataStatus(DataStatus::ListError, r.GetDesc());
          result = r;
        }
        f->SetType(i->GetType());
        f->SetMetaData("type", (i->GetType() == FileInfo::file_type_dir) ? "dir" : "file");
      }
      if (i->CheckSize()) {
        f->SetSize(i->GetSize());
        f->SetMetaData("size", tostring(i->GetSize()));
      }
      if (i->CheckCreated()) {
        f->SetCreated(i->GetCreated());
        f->SetMetaData("mtime", i->GetCreated());
      }
      if (i->CheckCheckSum()) {
        f->SetCheckSum(i->GetCheckSum());
        f->SetMetaData("checksum", i->GetCheckSum());
      }
    }
    return result;
  }

  DataPointGridFTP::DataPointGridFTP(const URL& url, const UserConfig& usercfg)
    : DataPointDirect(url, usercfg),
      ftp_active(false),
      credential(NULL),
      reading(false),
      writing(false),
      ftp_eof_flag(false),
      check_received_length(0) {
    //globus_module_activate(GLOBUS_FTP_CLIENT_MODULE);
    //if (!proxy_initialized)
    //  proxy_initialized = GlobusRecoverProxyOpenSSL();
    // Activating globus only once because it looks like 
    // deactivation of GLOBUS_FTP_CONTROL_MODULE is not
    // handled properly on Windows. This should not cause
    // problems (except for valgrind) because this plugin
    // is registered as persistent.
    if (!proxy_initialized) {
#ifdef HAVE_GLOBUS_THREAD_SET_MODEL
      globus_thread_set_model("pthread");
#endif
      GlobusPrepareGSSAPI();
      GlobusModuleActivate(GLOBUS_COMMON_MODULE);
      GlobusModuleActivate(GLOBUS_FTP_CLIENT_MODULE);
      proxy_initialized = GlobusRecoverProxyOpenSSL();
    }
    is_secure = false;
    if (url.Protocol() == "gsiftp")
      is_secure = true;
    if (!ftp_active) {
      GlobusResult res;
      globus_ftp_client_handleattr_t ftp_attr;
      if (!(res = globus_ftp_client_handleattr_init(&ftp_attr))) {
        logger.msg(ERROR,
                   "init_handle: globus_ftp_client_handleattr_init failed");
        logger.msg(ERROR, "Globus error: %s", res.str());
        ftp_active = false;
        return;
      }
#ifdef HAVE_GLOBUS_FTP_CLIENT_HANDLEATTR_SET_GRIDFTP2
      if (!(res = globus_ftp_client_handleattr_set_gridftp2(&ftp_attr,
                                                            GLOBUS_TRUE))) {
        globus_ftp_client_handleattr_destroy(&ftp_attr);
        logger.msg(ERROR, "init_handle: "
                   "globus_ftp_client_handleattr_set_gridftp2 failed");
        logger.msg(ERROR, "Globus error: %s", res.str());
        ftp_active = false;
        return;
      }
#endif
      if (!(res = globus_ftp_client_handle_init(&ftp_handle, &ftp_attr))) {
        globus_ftp_client_handleattr_destroy(&ftp_attr);
        logger.msg(ERROR, "init_handle: globus_ftp_client_handle_init failed");
        logger.msg(ERROR, "Globus error: %s", res.str());
        ftp_active = false;
        return;
      }
      globus_ftp_client_handleattr_destroy(&ftp_attr);
      if (!(res = globus_ftp_client_operationattr_init(&ftp_opattr))) {
        logger.msg(ERROR, "init_handle: "
                   "globus_ftp_client_operationattr_init failed");
        logger.msg(ERROR, "Globus error: %s", res.str());
        globus_ftp_client_handle_destroy(&ftp_handle);
        ftp_active = false;
        return;
      }
    }
    ftp_active = true;
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

  void DataPointGridFTP::set_attributes(void) {
    globus_ftp_control_parallelism_t paral;
    if (ftp_threads > 1) {
      paral.fixed.mode = GLOBUS_FTP_CONTROL_PARALLELISM_FIXED;
      paral.fixed.size = ftp_threads;
    }
    else {
      paral.fixed.mode = GLOBUS_FTP_CONTROL_PARALLELISM_NONE;
      paral.fixed.size = 1;
    }
    globus_ftp_client_operationattr_set_parallelism(&ftp_opattr, &paral);
    globus_ftp_client_operationattr_set_striped(&ftp_opattr, GLOBUS_FALSE);
    /*   globus_ftp_client_operationattr_set_layout         */
    /*   globus_ftp_client_operationattr_set_tcp_buffer     */
    globus_ftp_client_operationattr_set_type(&ftp_opattr,
                                             GLOBUS_FTP_CONTROL_TYPE_IMAGE);
    if (!is_secure) { // plain ftp protocol
      GlobusResult r = globus_ftp_client_operationattr_set_authorization(
                     &ftp_opattr,
                     GSS_C_NO_CREDENTIAL,
                     url.Username().empty() ? "anonymous" : url.Username().c_str(),
                     url.Passwd().empty() ? NULL : url.Passwd().c_str(),
                     GLOBUS_NULL, GLOBUS_NULL);
      if(!r) {
        logger.msg(VERBOSE, "globus_ftp_client_operationattr_set_authorization: error: %s", r.str());
      }

      globus_ftp_client_operationattr_set_mode(&ftp_opattr,
                                               GLOBUS_FTP_CONTROL_MODE_STREAM);
      globus_ftp_client_operationattr_set_data_protection(&ftp_opattr,
                                                          GLOBUS_FTP_CONTROL_PROTECTION_CLEAR);
      globus_ftp_client_operationattr_set_control_protection(&ftp_opattr,
                                                             GLOBUS_FTP_CONTROL_PROTECTION_CLEAR);
      // need to set dcau to none in order Globus libraries not to send
      // it to pure ftp server
      globus_ftp_control_dcau_t dcau;
      dcau.mode = GLOBUS_FTP_CONTROL_DCAU_NONE;
      globus_ftp_client_operationattr_set_dcau(&ftp_opattr, &dcau);
    }
    else { // gridftp protocol

      if (!credential)
        credential = new GSSCredential(usercfg.ProxyPath(),
                                       usercfg.CertificatePath(), usercfg.KeyPath());

      GlobusResult r = globus_ftp_client_operationattr_set_authorization(
                     &ftp_opattr,
                     *credential,":globus-mapping:","user@",
                     GLOBUS_NULL,GLOBUS_NULL);
      if(!r) {
        logger.msg(WARNING, "Failed to set credentials for GridFTP transfer");
        logger.msg(VERBOSE, "globus_ftp_client_operationattr_set_authorization: error: %s", r.str());
      }
      if (force_secure || (url.Option("secure") == "yes")) {
        globus_ftp_client_operationattr_set_mode(&ftp_opattr,
                                                 GLOBUS_FTP_CONTROL_MODE_EXTENDED_BLOCK);
        globus_ftp_client_operationattr_set_data_protection(&ftp_opattr,
                                                            GLOBUS_FTP_CONTROL_PROTECTION_PRIVATE);
        logger.msg(VERBOSE, "Using secure data transfer");
      }
      else {
        if (force_passive)
          globus_ftp_client_operationattr_set_mode(&ftp_opattr,
                                                   GLOBUS_FTP_CONTROL_MODE_STREAM);
        else
          globus_ftp_client_operationattr_set_mode(&ftp_opattr,
                                                   GLOBUS_FTP_CONTROL_MODE_EXTENDED_BLOCK);
        globus_ftp_client_operationattr_set_data_protection(&ftp_opattr,
                                                            GLOBUS_FTP_CONTROL_PROTECTION_CLEAR);
        logger.msg(VERBOSE, "Using insecure data transfer");
      }
      globus_ftp_client_operationattr_set_control_protection(&ftp_opattr,
                                                             GLOBUS_FTP_CONTROL_PROTECTION_PRIVATE);
    }
    /*   globus_ftp_client_operationattr_set_dcau                         */
    /*   globus_ftp_client_operationattr_set_resume_third_party_transfer  */
    /*   globus_ftp_client_operationattr_set_authorization                */
    globus_ftp_client_operationattr_set_append(&ftp_opattr, GLOBUS_FALSE);
  }

  DataPointGridFTP::~DataPointGridFTP() {
    StopReading();
    StopWriting();
    if (ftp_active) {
      logger.msg(DEBUG, "DataPoint::deinit_handle: destroy ftp_handle");
      // In case globus is still doing something asynchronously
      while(globus_ftp_client_handle_destroy(&ftp_handle) != GLOBUS_SUCCESS) {
        logger.msg(VERBOSE, "DataPoint::deinit_handle: destroy ftp_handle failed - retrying");
        // Unfortunately there is no sutable condition to wait for.
        // But such situation should happen very rarely if ever.
        sleep(1);
      }
      globus_ftp_client_operationattr_destroy(&ftp_opattr);
    }
    if (credential)
      delete credential;
    // See activation for description
    //globus_module_deactivate(GLOBUS_FTP_CLIENT_MODULE);
  }

  Plugin* DataPointGridFTP::Instance(PluginArgument *arg) {
    DataPointPluginArgument *dmcarg = dynamic_cast<DataPointPluginArgument*>(arg);
    if (!dmcarg)
      return NULL;
    if (((const URL&)(*dmcarg)).Protocol() != "gsiftp" &&
        ((const URL&)(*dmcarg)).Protocol() != "ftp")
      return NULL;
    // Make this code non-unloadable because both OpenSSL
    // and Globus have problems with unloading
    Glib::Module* module = dmcarg->get_module();
    PluginsFactory* factory = dmcarg->get_factory();
    if(!(factory && module)) {
      logger.msg(ERROR, "Missing reference to factory and/or module. It is unsafe to use Globus in non-persistent mode - (Grid)FTP code is disabled. Report to developers.");
      return NULL;
    }
    factory->makePersistent(module);
    OpenSSLInit();
    return new DataPointGridFTP(*dmcarg, *dmcarg);
  }

  bool DataPointGridFTP::WriteOutOfOrder() {
    return true;
  }

  bool DataPointGridFTP::ProvidesMeta() const {
    return true;
  }

  const std::string DataPointGridFTP::DefaultCheckSum() const {
    // no way to know which checksum is used for each file, so hard-code adler32 for now
    return std::string("adler32");
  }

  bool DataPointGridFTP::SetURL(const URL& url) {
    if ((url.Protocol() != "gsiftp") && (url.Protocol() != "ftp")) {
      return false;
    }
    // Globus FTP handle allows changing url completely
    this->url = url;
    return true;
  }
} // namespace Arc

Arc::PluginDescriptor PLUGINS_TABLE_NAME[] = {
  { "gridftp", "HED:DMC", 0, &Arc::DataPointGridFTP::Instance },
  { NULL, NULL, 0, NULL }
};
