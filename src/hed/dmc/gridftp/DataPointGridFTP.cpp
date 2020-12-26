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
#include <arc/globusutils/GlobusErrorUtils.h>
#include <arc/globusutils/GlobusWorkarounds.h>
#include <arc/globusutils/GSSCredential.h>
#include <arc/crypto/OpenSSL.h>

#include "DataPointGridFTP.h"
#include "Lister.h"

namespace ArcDMCGridFTP {

  using namespace Arc;

  static bool proxy_initialized = false;

  char dummy_buffer = 0;

  Logger DataPointGridFTP::logger(Logger::getRootLogger(), "DataPoint.GridFTP");

  void DataPointGridFTP::ftp_complete_callback(void *arg,
                                               globus_ftp_client_handle_t*,
                                               globus_object_t *error) {
    DataPointGridFTP *it = ((CBArg*)arg)->acquire();
    if(!it) return;
    if (error == GLOBUS_SUCCESS) {
      logger.msg(DEBUG, "ftp_complete_callback: success");
      it->callback_status = DataStatus::Success;
      it->cond.signal();
    }
    else {
      std::string err(trim(globus_object_to_string(error)));
      logger.msg(VERBOSE, "ftp_complete_callback: error: %s", err);
      it->callback_status = DataStatus(DataStatus::GenericError, globus_error_to_errno(err, EARCOTHER), err);
      it->cond.signal();
    }
    ((CBArg*)arg)->release();
  }

  void DataPointGridFTP::ftp_check_callback(void *arg,
                                            globus_ftp_client_handle_t*,
                                            globus_object_t *error,
                                            globus_byte_t*,
                                            globus_size_t length,
                                            globus_off_t,
                                            globus_bool_t eof) {
    DataPointGridFTP *it = ((CBArg*)arg)->acquire();
    if(!it) return;
    logger.msg(VERBOSE, "ftp_check_callback");
    if (error != GLOBUS_SUCCESS) {
      logger.msg(VERBOSE, "Globus error: %s", globus_object_to_string(error));
      ((CBArg*)arg)->release();
      return;
    }
    if (eof) {
      it->ftp_eof_flag = true;
      ((CBArg*)arg)->release();
      return;
    }
    if (it->check_received_length > 0) {
      logger.msg(INFO,
                 "Excessive data received while checking file access");
      it->ftp_eof_flag = true;
      GlobusResult(globus_ftp_client_abort(&(it->ftp_handle)));
      ((CBArg*)arg)->release();
      return;
    }
    it->check_received_length += length;
    ((CBArg*)arg)->release();
    GlobusResult res(globus_ftp_client_register_read(&(it->ftp_handle),
                                      (globus_byte_t*)(it->ftp_buf),
                                      sizeof(it->ftp_buf),
                                      &ftp_check_callback, arg));
    it = ((CBArg*)arg)->acquire();
    if(!it) return;
    if (!res) {
      logger.msg(INFO,
                 "Registration of Globus FTP buffer failed - cancel check");
      logger.msg(VERBOSE, "Globus error: %s", res.str());
      GlobusResult(globus_ftp_client_abort(&(it->ftp_handle)));
      ((CBArg*)arg)->release();
      return;
    }
    ((CBArg*)arg)->release();
    return;
  }

  DataStatus DataPointGridFTP::Check(bool check_meta) {
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
    if (check_meta) {
      res = globus_ftp_client_size(&ftp_handle, url.plainstr().c_str(), &ftp_opattr,
                                   &size, &ftp_complete_callback, cbarg);
      if (!res) {
        logger.msg(VERBOSE, "check_ftp: globus_ftp_client_size failed");
        logger.msg(INFO, "Globus error: %s", res.str());
      }
      else if (!cond.wait(1000*usercfg.Timeout())) {
        logger.msg(INFO, "check_ftp: timeout waiting for size");
        GlobusResult(globus_ftp_client_abort(&ftp_handle));
        cond.wait();
      }
      else if (!callback_status)
        logger.msg(INFO, "check_ftp: failed to get file's size");
      else {
        SetSize(size);
        logger.msg(VERBOSE, "check_ftp: obtained size: %lli", GetSize());
      }
      res = globus_ftp_client_modification_time(&ftp_handle, url.plainstr().c_str(),
                                                &ftp_opattr, &gl_modify_time,
                                                &ftp_complete_callback, cbarg);
      if (!res) {
        logger.msg(VERBOSE,
                   "check_ftp: globus_ftp_client_modification_time failed");
        logger.msg(INFO, "Globus error: %s", res.str());
      }
      else if (!cond.wait(1000*usercfg.Timeout())) {
        logger.msg(INFO, "check_ftp: timeout waiting for modification_time");
        GlobusResult(globus_ftp_client_abort(&ftp_handle));
        cond.wait();
      }
      else if (!callback_status)
        logger.msg(INFO, "check_ftp: failed to get file's modification time");
      else {
        int modify_utime;
        GlobusTimeAbstimeGet(gl_modify_time, modify_time, modify_utime);
        SetModified(modify_time);
        logger.msg(VERBOSE, "check_ftp: obtained modification date: %s", GetModified().str());
      }
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
      res = globus_ftp_client_partial_get(&ftp_handle, url.plainstr().c_str(),
                                          &ftp_opattr, GLOBUS_NULL, 0, 1,
                                          &ftp_complete_callback, cbarg);
      if (!res) {
        std::string globus_err(res.str());
        logger.msg(VERBOSE, "check_ftp: globus_ftp_client_get failed");
        logger.msg(VERBOSE, globus_err);
        return DataStatus(DataStatus::CheckError, globus_err);
      }
      // use eof_flag to pass result from callback
      ftp_eof_flag = false;
      check_received_length = 0;
      logger.msg(VERBOSE, "check_ftp: globus_ftp_client_register_read");
      res = globus_ftp_client_register_read(&ftp_handle,
                                            (globus_byte_t*)ftp_buf,
                                            sizeof(ftp_buf),
                                            &ftp_check_callback, cbarg);
      if (!res) {
        GlobusResult(globus_ftp_client_abort(&ftp_handle));
        cond.wait();
        return DataStatus::CheckError;
      }
      if (!cond.wait(1000*usercfg.Timeout())) {
        logger.msg(VERBOSE, "check_ftp: timeout waiting for partial get");
        GlobusResult(globus_ftp_client_abort(&ftp_handle));
        cond.wait();
        return DataStatus(DataStatus::CheckError, EARCREQUESTTIMEOUT,
                          "timeout waiting for partial get from server: "+url.plainstr());
      }
      if (ftp_eof_flag) return DataStatus::Success;
      return DataStatus(DataStatus::CheckError, callback_status.GetDesc());
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
    if (!ftp_active) return DataStatus::NotInitializedError;
    if (reading) return DataStatus::IsReadingError;
    if (writing) return DataStatus::IsWritingError;
    GlobusResult res;
    set_attributes();

    // Try file delete and then dir delete if that fails. It would be good to
    // use EISDIR but we cannot rely on that error being detected properly for
    // all server implementations.
    DataStatus rm_res = RemoveFile();
    if (!rm_res && rm_res.GetErrno() != ENOENT && rm_res.GetErrno() != EACCES) {
      logger.msg(INFO, "File delete failed, attempting directory delete for %s", url.plainstr());
      rm_res = RemoveDir();
    }
    return rm_res;
  }

  DataStatus DataPointGridFTP::RemoveFile() {
    GlobusResult res(globus_ftp_client_delete(&ftp_handle, url.plainstr().c_str(),
                                   &ftp_opattr, &ftp_complete_callback, cbarg));
    if (!res) {
      logger.msg(VERBOSE, "delete_ftp: globus_ftp_client_delete failed");
      std::string globus_err(res.str());
      logger.msg(VERBOSE, globus_err);
      return DataStatus(DataStatus::DeleteError, globus_err);
    }
    if (!cond.wait(1000*usercfg.Timeout())) {
      logger.msg(VERBOSE, "delete_ftp: timeout waiting for delete");
      GlobusResult(globus_ftp_client_abort(&ftp_handle));
      cond.wait();
      return DataStatus(DataStatus::DeleteError, EARCREQUESTTIMEOUT, "Timeout waiting for delete for "+url.plainstr());
    }
    if (!callback_status) {
      return DataStatus(DataStatus::DeleteError, callback_status.GetErrno(), callback_status.GetDesc());
    }
    return DataStatus::Success;
  }

  DataStatus DataPointGridFTP::RemoveDir() {
    GlobusResult res(globus_ftp_client_rmdir(&ftp_handle, url.plainstr().c_str(),
                                  &ftp_opattr, &ftp_complete_callback, cbarg));
    if (!res) {
      logger.msg(VERBOSE, "delete_ftp: globus_ftp_client_rmdir failed");
      std::string globus_err(res.str());
      logger.msg(VERBOSE, globus_err);
      return DataStatus(DataStatus::DeleteError, globus_err);
    }
    if (!cond.wait(1000*usercfg.Timeout())) {
      logger.msg(VERBOSE, "delete_ftp: timeout waiting for delete");
      GlobusResult(globus_ftp_client_abort(&ftp_handle));
      cond.wait();
      return DataStatus(DataStatus::DeleteError, EARCREQUESTTIMEOUT, "Timeout waiting for delete of "+url.plainstr());
    }
    if (!callback_status) {
      return DataStatus(DataStatus::DeleteError, callback_status.GetErrno(), callback_status.GetDesc());
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
    std::string ftp_dir_path = url.plainstr();
    for (;;)
      if (!remove_last_dir(ftp_dir_path))
        break;
    bool result = true;
    for (;;) {
      if (!add_last_dir(ftp_dir_path, url.plainstr()))
        break;
      logger.msg(VERBOSE, "mkdir_ftp: making %s", ftp_dir_path);
      GlobusResult res(globus_ftp_client_mkdir(&ftp_handle, ftp_dir_path.c_str(), &ftp_opattr,
                                &ftp_complete_callback, cbarg));
      if (!res) {
        logger.msg(INFO, "Globus error: %s", res.str());
        return false;
      }
      if (!cond.wait(1000*usercfg.Timeout())) {
        logger.msg(INFO, "mkdir_ftp: timeout waiting for mkdir");
        /* timeout - have to cancel operation here */
        GlobusResult(globus_ftp_client_abort(&ftp_handle));
        cond.wait();
        return false;
      }
      if (!callback_status)
        result = false;
    }
    return result;
  }

  DataStatus DataPointGridFTP::CreateDirectory(bool with_parents) {
    if (!ftp_active)
      return DataStatus::NotInitializedError;
    set_attributes();

    // if with_parents use standard method used during StartWriting
    if (with_parents)
      return mkdir_ftp() ? DataStatus::Success : DataStatus::CreateDirectoryError;

    // the globus mkdir call uses the full URL
    std::string dirpath = url.plainstr();
    // check if file is in root directory
    if (!remove_last_dir(dirpath)) return DataStatus::Success;

    logger.msg(VERBOSE, "Creating directory %s", dirpath);
    GlobusResult res(globus_ftp_client_mkdir(&ftp_handle, dirpath.c_str(), &ftp_opattr,
                              &ftp_complete_callback, cbarg));
    if (!res) {
      std::string err(res.str());
      logger.msg(VERBOSE, "Globus error: %s", err);
      return DataStatus(DataStatus::CreateDirectoryError, err);
    }
    if (!cond.wait(1000*usercfg.Timeout())) {
      logger.msg(VERBOSE, "Timeout waiting for mkdir");
      /* timeout - have to cancel operation here */
      GlobusResult(globus_ftp_client_abort(&ftp_handle));
      cond.wait();
      return DataStatus(DataStatus::CreateDirectoryError, EARCREQUESTTIMEOUT,
                        "Timeout waiting for mkdir at "+url.plainstr());
    }
    if (!callback_status) {
      return DataStatus(DataStatus::CreateDirectoryError, callback_status.GetErrno(), callback_status.GetDesc());
    }
    return DataStatus::Success;
  }

  DataStatus DataPointGridFTP::StartReading(DataBuffer& buf) {
    if (!ftp_active) return DataStatus::NotInitializedError;
    if (reading) return DataStatus::IsReadingError;
    if (writing) return DataStatus::IsWritingError;
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
    GlobusResult(globus_ftp_client_handle_cache_url_state(&ftp_handle, url.plainstr().c_str()));
    GlobusResult res;
    logger.msg(VERBOSE, "start_reading_ftp: globus_ftp_client_get");
    cond.reset();
    if (limit_length) {
      res = globus_ftp_client_partial_get(&ftp_handle, url.plainstr().c_str(),
                                          &ftp_opattr, GLOBUS_NULL,
                                          range_start,
                                          range_start + range_length + 1,
                                          &ftp_get_complete_callback, cbarg);
    } else {
      res = globus_ftp_client_get(&ftp_handle, url.plainstr().c_str(),
                                  &ftp_opattr, GLOBUS_NULL,
                                  &ftp_get_complete_callback, cbarg);
    }
    if (!res) {
      logger.msg(VERBOSE, "start_reading_ftp: globus_ftp_client_get failed");
      std::string globus_err(res.str());
      logger.msg(VERBOSE, globus_err);

      GlobusResult(globus_ftp_client_handle_flush_url_state(&ftp_handle, url.plainstr().c_str()));
      buffer->error_read(true);
      reading = false;
      return DataStatus(DataStatus::ReadStartError, globus_err);
    }
    if (!GlobusResult(globus_thread_create(&ftp_control_thread, GLOBUS_NULL,
                             &ftp_read_thread, this))) {
      logger.msg(VERBOSE, "start_reading_ftp: globus_thread_create failed");
      GlobusResult(globus_ftp_client_abort(&ftp_handle));
      cond.wait();
      GlobusResult(globus_ftp_client_handle_flush_url_state(&ftp_handle, url.plainstr().c_str()));
      buffer->error_read(true);
      reading = false;
      return DataStatus(DataStatus::ReadStartError, "Failed to create new thread");
    }
    // make sure globus has thread for handling network/callbacks
    GlobusResult(globus_thread_blocking_will_block());
    return DataStatus::Success;
  }

  DataStatus DataPointGridFTP::StopReading() {
    if (!reading) return DataStatus::ReadStopError;
    reading = false;
    // If error in buffer then read thread will already have called abort
    if (!buffer) return DataStatus::Success;
    if (!buffer->eof_read() && !buffer->error()) {
      logger.msg(VERBOSE, "stop_reading_ftp: aborting connection");
      GlobusResult res(globus_ftp_client_abort(&ftp_handle));
      if(!res) {
        // This mostly means transfer failed and Globus did not call complete 
        // callback. Because it was reported that Globus may call it even
        // 1 hour after abort initiated here that callback is imitated.
        std::string globus_err(res.str());
        logger.msg(INFO, "Failed to abort transfer of ftp file: %s", globus_err);
        logger.msg(INFO, "Assuming transfer is already aborted or failed.");
        cond.lock();
        failure_code = DataStatus(DataStatus::ReadStopError, globus_err);
        cond.unlock();
        buffer->error_read(true);
      }
    }
    logger.msg(VERBOSE, "stop_reading_ftp: waiting for transfer to finish");
    cond.wait();
    logger.msg(VERBOSE, "stop_reading_ftp: exiting: %s", url.plainstr());
    //GlobusResult(globus_ftp_client_handle_flush_url_state(&ftp_handle, url.plainstr().c_str()));
    if (!callback_status) return DataStatus(DataStatus::ReadStopError, callback_status.GetDesc());
    return DataStatus::Success;
  }

  void* DataPointGridFTP::ftp_read_thread(void *arg) {
    DataPointGridFTP *it = (DataPointGridFTP*)arg;
    int h;
    unsigned int l;
    GlobusResult res;
    int registration_failed = 0;
    it->data_error = false;
    it->data_counter.set(0);
    logger.msg(INFO, "ftp_read_thread: get and register buffers");
    int n_buffers = 0;
    for (;;) {
      if (it->buffer->eof_read()) break;
      if (!it->buffer->for_read(h, l, true)) { /* eof or error */
        if (it->buffer->error()) { /* error -> abort reading */
          logger.msg(VERBOSE, "ftp_read_thread: for_read failed - aborting: %s",
                     it->url.plainstr());
          GlobusResult(globus_ftp_client_abort(&(it->ftp_handle)));
        }
        break;
      }
      if (it->data_error) {
        // This is meant to reduce time window for globus bug.
        // See comment in ftp_write_thread.
        it->buffer->is_read(h, 0, 0);
        logger.msg(VERBOSE, "ftp_read_thread: data callback failed - aborting: %s",
                   it->url.plainstr());
        GlobusResult(globus_ftp_client_abort(&(it->ftp_handle)));
        break;
      }
      it->data_counter.inc();
      res =
        globus_ftp_client_register_read(&(it->ftp_handle),
                                        (globus_byte_t*)((*(it->buffer))[h]),
                                        l, &(it->ftp_read_callback), it->cbarg);
      if (!res) {
        it->data_counter.dec();
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
                     it->url.plainstr());
        }
        else {
          logger.msg(DEBUG, "ftp_read_thread: "
                     "failed to register Globus buffer - will try later: %s",
                     it->url.plainstr());
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
    //if(!it->buffer->wait_for_read(15)) {
    if(!it->data_counter.wait(15)) {
      // See comment in ftp_write_thread for explanation.
      logger.msg(VERBOSE, "ftp_read_thread: failed to release buffers - leaking");
      CBArg* cbarg_old = it->cbarg;
      it->cbarg = new CBArg(it);
      cbarg_old->abandon();
    };
    logger.msg(VERBOSE, "ftp_read_thread: exiting");
    it->callback_status = it->buffer->error_read() ? DataStatus::ReadError :
                          DataStatus::Success;
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
    DataPointGridFTP *it = ((CBArg*)arg)->acquire();
    if(!it) return;
    if (error != GLOBUS_SUCCESS) {
      it->data_error = true;
      logger.msg(VERBOSE, "ftp_read_callback: failure: %s",globus_object_to_string(error));
      it->buffer->is_read((char*)buffer, 0, 0);
    } else {
      logger.msg(DEBUG, "ftp_read_callback: success");
      it->buffer->is_read((char*)buffer, length, offset);
      if (eof) it->ftp_eof_flag = true;
    }
    it->data_counter.dec();
    ((CBArg*)arg)->release();
    return;
  }

  void DataPointGridFTP::ftp_get_complete_callback(void *arg,
                                                   globus_ftp_client_handle_t*,
                                                   globus_object_t *error) {
    DataPointGridFTP *it = ((CBArg*)arg)->acquire();
    if(!it) return;
    /* data transfer finished */
    if (error != GLOBUS_SUCCESS) {
      logger.msg(INFO, "Failed to get ftp file");
      std::string err(trim(globus_object_to_string(error)));
      logger.msg(VERBOSE, "%s", err);
      it->cond.lock();
      it->failure_code = DataStatus(DataStatus::ReadStartError, globus_error_to_errno(err, EARCOTHER), err);
      it->cond.unlock();
      it->buffer->error_read(true);
    } else {
      it->buffer->eof_read(true); // This also reports to working threads transfer finished
    }
    ((CBArg*)arg)->release();
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
    GlobusResult(globus_ftp_client_handle_cache_url_state(&ftp_handle, url.plainstr().c_str()));
    if (autodir) {
      logger.msg(VERBOSE, "start_writing_ftp: mkdir");
      if (!mkdir_ftp())
        logger.msg(VERBOSE,
                   "start_writing_ftp: mkdir failed - still trying to write");
    }
    logger.msg(VERBOSE, "start_writing_ftp: put");
    cond.reset();
    if (limit_length) {
      res = globus_ftp_client_partial_put(&ftp_handle, url.plainstr().c_str(),
                                          &ftp_opattr, GLOBUS_NULL,
                                          range_start,
                                          range_start + range_length,
                                          &ftp_put_complete_callback, cbarg);
    } else {
      res = globus_ftp_client_put(&ftp_handle, url.plainstr().c_str(),
                                  &ftp_opattr, GLOBUS_NULL,
                                  &ftp_put_complete_callback, cbarg);
    }
    if (!res) {
      logger.msg(VERBOSE, "start_writing_ftp: put failed");
      std::string globus_err(res.str());
      logger.msg(VERBOSE, globus_err);
      GlobusResult(globus_ftp_client_handle_flush_url_state(&ftp_handle, url.plainstr().c_str()));
      buffer->error_write(true);
      writing = false;
      return DataStatus(DataStatus::WriteStartError, globus_err);
    }
    if (!GlobusResult(globus_thread_create(&ftp_control_thread, GLOBUS_NULL,
                             &ftp_write_thread, this))) {
      logger.msg(VERBOSE, "start_writing_ftp: globus_thread_create failed");
      GlobusResult(globus_ftp_client_handle_flush_url_state(&ftp_handle, url.plainstr().c_str()));
      buffer->error_write(true);
      writing = false;
      return DataStatus(DataStatus::WriteStartError, "Failed to create new thread");
    }
    // make sure globus has thread for handling network/callbacks
    GlobusResult(globus_thread_blocking_will_block());
    return DataStatus::Success;
  }

  DataStatus DataPointGridFTP::StopWriting() {
    if (!writing) return DataStatus::WriteStopError;
    writing = false;
    // If error in buffer then write thread will already have called abort
    if (!buffer) return DataStatus::Success;
    if (!buffer->eof_write() && !buffer->error()) {
      logger.msg(VERBOSE, "StopWriting: aborting connection");
      GlobusResult res(globus_ftp_client_abort(&ftp_handle));
      if(!res) {
        // This mostly means transfer failed and Globus did not call complete 
        // callback. Because it was reported that Globus may call it even
        // 1 hour after abort initiated here that callback is imitated.
        std::string globus_err(res.str());
        logger.msg(INFO, "Failed to abort transfer of ftp file: %s", globus_err);
        logger.msg(INFO, "Assuming transfer is already aborted or failed.");
        cond.lock();
        failure_code = DataStatus(DataStatus::WriteStopError, globus_err);
        cond.unlock();
        buffer->error_write(true);
      }
    }
    // Waiting for data transfer thread to finish
    cond.wait();
    // checksum verification
    const CheckSum * calc_sum = buffer->checksum_object();
    if (!buffer->error() && calc_sum && *calc_sum && buffer->checksum_valid()) {
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
          char cksum[256];
          std::string cksumtype(upper(DefaultCheckSum()));
          GlobusResult res(globus_ftp_client_cksm(&ftp_handle, url.plainstr().c_str(),
                                                    &ftp_opattr, cksum, (globus_off_t)0,
                                                    (globus_off_t)-1, cksumtype.c_str(),
                                                    &ftp_complete_callback, cbarg));
          if (!res) {
            logger.msg(VERBOSE, "list_files_ftp: globus_ftp_client_cksm failed");
            logger.msg(VERBOSE, "Globus error: %s", res.str());
          }
          else if (!cond.wait(1000*usercfg.Timeout())) {
            logger.msg(VERBOSE, "list_files_ftp: timeout waiting for cksum");
            GlobusResult(globus_ftp_client_abort(&ftp_handle));
            cond.wait();
          }
          else if (!callback_status) {
            // reset to success since failing to get checksum should not trigger an error
            callback_status = DataStatus::Success;
            logger.msg(INFO, "list_files_ftp: no checksum information possible");
          }
          else {
            logger.msg(VERBOSE, "list_files_ftp: checksum %s", cksum);
            if (csum.substr(csum.find(':')+1).length() != std::string(cksum).length()) {
              // Some buggy Globus servers return a different type of checksum to the one requested
              logger.msg(WARNING, "Checksum type returned by server is different to requested type, cannot compare");
            } else if (csum.substr(csum.find(':')+1) == std::string(cksum)) {
              logger.msg(INFO, "Calculated checksum %s matches checksum reported by server", csum);
              SetCheckSum(csum);
            } else {
              logger.msg(VERBOSE, "Checksum mismatch between calculated checksum %s and checksum reported by server %s",
                       csum, std::string(DefaultCheckSum()+':'+cksum));
              return DataStatus(DataStatus::TransferError, EARCCHECKSUM, "Checksum mismatch between calculated and reported checksums");
            }
          }
        }
      }
    }
    //GlobusResult(globus_ftp_client_handle_flush_url_state(&ftp_handle, url.plainstr().c_str()));
    if (!callback_status) return DataStatus(DataStatus::WriteStopError, callback_status.GetDesc());
    return DataStatus::Success;
  }

  void* DataPointGridFTP::ftp_write_thread(void *arg) {
    DataPointGridFTP *it = (DataPointGridFTP*)arg;
    int h;
    unsigned int l;
    unsigned long long int o;
    GlobusResult res;
    globus_bool_t eof = GLOBUS_FALSE;
    it->data_error = false;
    it->data_counter.set(0);
    logger.msg(INFO, "ftp_write_thread: get and register buffers");
    for (;;) {
      if (!it->buffer->for_write(h, l, o, true)) {
        if (it->buffer->error()) {
          logger.msg(VERBOSE, "ftp_write_thread: for_write failed - aborting");
          GlobusResult(globus_ftp_client_abort(&(it->ftp_handle)));
          break;
        }
        // no buffers and no errors - must be pure eof
        eof = GLOBUS_TRUE;
        o = it->buffer->eof_position();
        res = globus_ftp_client_register_write(&(it->ftp_handle),
                                               (globus_byte_t*)(&dummy_buffer), 0, o,
                                               eof, &(it->ftp_write_callback), it->cbarg);
        break;
        // if(res == GLOBUS_SUCCESS) break;
        // sleep(1); continue;
      }
      if (it->data_error) {
        // This is meant to reduce time window for globus bug.
        // See comment below about data_counter.
        it->buffer->is_notwritten(h);
        logger.msg(VERBOSE, "ftp_write_thread: data callback failed - aborting");
        GlobusResult(globus_ftp_client_abort(&(it->ftp_handle)));
        break;
      }
      it->data_counter.inc();
      res =
        globus_ftp_client_register_write(&(it->ftp_handle),
                                         (globus_byte_t*)((*(it->buffer))[h]),
                                         l, o, eof, &(it->ftp_write_callback), it->cbarg);
      if (!res) {
        it->data_counter.dec();
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
    // if that does not happen quickly that means there are problems.
    //if(!it->buffer->wait_for_write(15)) {
    if(!it->data_counter.wait(15000)) {
      // If buffer registration happens while globus is reporting error
      // those buffers are lost by globus. But still we can't be sure
      // callback is never called. So switching to new cbarg to detach
      // potential callbacks from this object.
      logger.msg(VERBOSE, "ftp_write_thread: failed to release buffers - leaking");
      CBArg* cbarg_old = it->cbarg;
      it->cbarg = new CBArg(it);
      cbarg_old->abandon();
    };
    logger.msg(VERBOSE, "ftp_write_thread: exiting");
    it->callback_status = it->buffer->error_write() ? DataStatus::WriteError :
                         DataStatus::Success;
    it->cond.signal(); // Report to control thread that data transfer thread finished
    return NULL;
  }

  void DataPointGridFTP::ftp_write_callback(void *arg,
                                            globus_ftp_client_handle_t*,
                                            globus_object_t *error,
                                            globus_byte_t *buffer,
                                            globus_size_t,
                                            globus_off_t,
                                            globus_bool_t is_eof) {
    DataPointGridFTP *it = ((CBArg*)arg)->acquire();
    if(!it) return;
    // Filtering out dummy write - doing that to avoid additional check for dummy write complete
    if(buffer == (globus_byte_t*)(&dummy_buffer)) {
      ((CBArg*)arg)->release();
      return;
    }
    if (error != GLOBUS_SUCCESS) {
      it->data_error = true;
      logger.msg(VERBOSE, "ftp_write_callback: failure: %s",globus_object_to_string(error));
      it->buffer->is_notwritten((char*)buffer);
    } else {
      logger.msg(DEBUG, "ftp_write_callback: success %s",is_eof?"eof":"   ");
      it->buffer->is_written((char*)buffer);
    }
    it->data_counter.dec();
    ((CBArg*)arg)->release();
    return;
  }

  void DataPointGridFTP::ftp_put_complete_callback(void *arg,
                                                   globus_ftp_client_handle_t*,
                                                   globus_object_t *error) {
    DataPointGridFTP *it = ((CBArg*)arg)->acquire();
    if(!it) return;
    /* data transfer finished */
    if (error != GLOBUS_SUCCESS) {
      logger.msg(INFO, "Failed to store ftp file");
      std::string err(trim(globus_object_to_string(error)));
      logger.msg(VERBOSE, "%s", err);
      it->cond.lock(); // Protect access to failure_code
      it->failure_code = DataStatus(DataStatus::WriteStartError, globus_error_to_errno(err, EARCOTHER), err);
      it->cond.unlock();
      it->buffer->error_write(true);
    } else {
      logger.msg(DEBUG, "ftp_put_complete_callback: success");
      // This also reports to data transfer thread that transfer finished
      it->buffer->eof_write(true);
    }
    ((CBArg*)arg)->release();
    return;
  }

  DataStatus DataPointGridFTP::do_more_stat(FileInfo& f, DataPointInfoType verb) {
    DataStatus result = DataStatus::Success;
    GlobusResult res;
    globus_off_t size = 0;
    globus_abstime_t gl_modify_time;
    time_t modify_time;
    std::string f_url = url.ConnectionURL() + f.GetName();
    if (((verb & INFO_TYPE_CONTENT) == INFO_TYPE_CONTENT) && (!f.CheckSize()) && (f.GetType() != FileInfo::file_type_dir)) {
      logger.msg(DEBUG, "list_files_ftp: looking for size of %s", f_url);
      res = globus_ftp_client_size(&ftp_handle, f_url.c_str(), &ftp_opattr,
                                   &size, &ftp_complete_callback, cbarg);
      if (!res) {
        logger.msg(VERBOSE, "list_files_ftp: globus_ftp_client_size failed");
        std::string globus_err(res.str());
        logger.msg(INFO, "Globus error: %s", globus_err);
        result = DataStatus(DataStatus::StatError, globus_err);
      }
      else if (!cond.wait(1000*usercfg.Timeout())) {
        logger.msg(INFO, "list_files_ftp: timeout waiting for size");
        logger.msg(INFO, "list_files_ftp: timeout waiting for size");
        GlobusResult(globus_ftp_client_abort(&ftp_handle));
        cond.wait();
        result = DataStatus(DataStatus::StatError, EARCREQUESTTIMEOUT, "timeout waiting for file size");
      }
      else if (!callback_status) {
        logger.msg(INFO, "list_files_ftp: failed to get file's size");
        result = DataStatus(DataStatus::StatError, callback_status.GetDesc());
        // Guessing - directories usually have no size
        f.SetType(FileInfo::file_type_dir);
      }
      else {
        f.SetSize(size);
        // Guessing - only files usually have size
        f.SetType(FileInfo::file_type_file);
      }
    }
    if ((verb & INFO_TYPE_TIMES) == INFO_TYPE_TIMES && !f.CheckModified()) {
      logger.msg(DEBUG, "list_files_ftp: "
                        "looking for modification time of %s", f_url);
      res = globus_ftp_client_modification_time(&ftp_handle, f_url.c_str(),
                                                &ftp_opattr, &gl_modify_time,
                                                &ftp_complete_callback, cbarg);
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
        GlobusResult(globus_ftp_client_abort(&ftp_handle));
        cond.wait();
        result = DataStatus(DataStatus::StatError, EARCREQUESTTIMEOUT,
                            "timeout waiting for file modification time");
      }
      else if (!callback_status) {
        logger.msg(INFO, "list_files_ftp: "
                         "failed to get file's modification time");
        result = DataStatus(DataStatus::StatError, callback_status.GetDesc());
      }
      else {
        int modify_utime;
        GlobusTimeAbstimeGet(gl_modify_time, modify_time, modify_utime);
        f.SetModified(modify_time);
      }
    }
    if ((verb & INFO_TYPE_CONTENT) == INFO_TYPE_CONTENT && !f.CheckCheckSum() && f.GetType() != FileInfo::file_type_dir) {
      // not all implementations support checksum so failure is not an error
      logger.msg(DEBUG, "list_files_ftp: "
                        "looking for checksum of %s", f_url);
      char cksum[256];
      std::string cksumtype(upper(DefaultCheckSum()).c_str());
      res = globus_ftp_client_cksm(&ftp_handle, f_url.c_str(),
                                   &ftp_opattr, cksum, (globus_off_t)0,
                                   (globus_off_t)-1, cksumtype.c_str(),
                                   &ftp_complete_callback, cbarg);
      if (!res) {
        logger.msg(VERBOSE, "list_files_ftp: globus_ftp_client_cksm failed");
        logger.msg(VERBOSE, "Globus error: %s", res.str());
      }
      else if (!cond.wait(1000*usercfg.Timeout())) {
        logger.msg(VERBOSE, "list_files_ftp: timeout waiting for cksum");
        GlobusResult(globus_ftp_client_abort(&ftp_handle));
        cond.wait();
      }
      else if (!callback_status) {
        // reset to success since failing to get checksum should not trigger an error
        callback_status = DataStatus::Success;
        logger.msg(INFO, "list_files_ftp: no checksum information possible");
      }
      else {
        logger.msg(VERBOSE, "list_files_ftp: checksum %s", cksum);
        f.SetCheckSum(DefaultCheckSum() + ':' + std::string(cksum));
      }
    }
    return result;
  }

  DataStatus DataPointGridFTP::Stat(FileInfo& file, DataPoint::DataPointInfoType verb) {
    if (!ftp_active) return DataStatus::NotInitializedError;
    if (reading) return DataStatus::IsReadingError;
    if (writing) return DataStatus::IsWritingError;
    reading = true;
    set_attributes();
    bool more_info = ((verb | INFO_TYPE_NAME) != INFO_TYPE_NAME);
    DataStatus lister_res = lister->retrieve_file_info(url,!more_info);
    if (!lister_res) {
      logger.msg(VERBOSE, "Failed to obtain stat from FTP: %s", lister_res.GetDesc());
      reading = false;
      return lister_res;
    }
    DataStatus result = DataStatus::StatError;
    if (lister->size() == 0) {
      logger.msg(VERBOSE, "No results returned from stat");
      result.SetDesc("No results found for "+url.plainstr());
      reading = false;
      return result;
    }
    if(lister->size() != 1) {
      logger.msg(VERBOSE, "Wrong number of objects (%i) for stat from ftp: %s", lister->size(), url.plainstr());
      // guess - that probably means it is directory 
      file.SetName(FileInfo(url.Path()).GetName());
      file.SetType(FileInfo::file_type_dir);
      reading = false;
      return DataStatus::Success;
    }
    FileInfo lister_info(*(lister->begin()));
    // does returned path match what we expect?
    // remove trailing slashes from url
    std::string fname(url.Path());
    while (fname.length() > 1 && fname[fname.length()-1] == '/') fname.erase(fname.length()-1);
    if ((lister_info.GetName().substr(lister_info.GetName().rfind('/')+1)) !=
              (fname.substr(fname.rfind('/')+1))) {
      logger.msg(VERBOSE, "Unexpected path %s returned from server", lister_info.GetName());
      result.SetDesc("Unexpected path returned from server for "+url.plainstr());
      reading = false;
      return result;
    }
    result = DataStatus::Success;
    if (lister_info.GetName()[0] != '/') lister_info.SetName(url.Path());

    file.SetName(lister_info.GetName());
    if (more_info) {
      DataStatus r = do_more_stat(lister_info, verb);
      if(!r) result = r;
    }
    file.SetType(lister_info.GetType());
    if (lister_info.CheckSize()) {
      file.SetSize(lister_info.GetSize());
      SetSize(lister_info.GetSize());
    }
    if (lister_info.CheckModified()) {
      file.SetModified(lister_info.GetModified());
      SetModified(lister_info.GetModified());
    }
    if (lister_info.CheckCheckSum()) {
      file.SetCheckSum(lister_info.GetCheckSum());
      SetCheckSum(lister_info.GetCheckSum());
    }
    reading = false;
    return result;
  }

  DataStatus DataPointGridFTP::List(std::list<FileInfo>& files, DataPoint::DataPointInfoType verb) {
    if (!ftp_active) return DataStatus::NotInitializedError;
    if (reading) return DataStatus::IsReadingError;
    if (writing) return DataStatus::IsWritingError;
    reading = true;
    set_attributes();
    bool more_info = ((verb | INFO_TYPE_NAME) != INFO_TYPE_NAME);
    DataStatus lister_res = lister->retrieve_dir_info(url,!more_info);
    if (!lister_res) {
      logger.msg(VERBOSE, "Failed to obtain listing from FTP: %s", lister_res.GetDesc());
      reading = false;
      return lister_res;
    }
    DataStatus result = DataStatus::Success;
    for (std::list<FileInfo>::iterator i = lister->begin();
         i != lister->end(); ++i) {
      if (i->GetName()[0] != '/')
        i->SetName(url.Path()+'/'+i->GetName());
      std::list<FileInfo>::iterator f =
        files.insert(files.end(), FileInfo(i->GetLastName()));
      if (more_info) {
        DataStatus r = do_more_stat(*i, verb);
        if(!r) {
          if(r == DataStatus::StatError) r = DataStatus(DataStatus::ListError, r.GetDesc());
          result = r;
        }
        f->SetType(i->GetType());
      }
      if (i->CheckSize()) f->SetSize(i->GetSize());
      if (i->CheckModified()) f->SetModified(i->GetModified());
      if (i->CheckCheckSum()) f->SetCheckSum(i->GetCheckSum());
    }
    reading = false;
    return result;
  }

  DataStatus DataPointGridFTP::Rename(const URL& newurl) {
    if (!ftp_active)
      return DataStatus::NotInitializedError;
    if (reading)
      return DataStatus::IsReadingError;
    if (writing)
      return DataStatus::IsWritingError;
    set_attributes();

    GlobusResult res(globus_ftp_client_move(&ftp_handle,
                                              url.plainstr().c_str(),
                                              newurl.plainstr().c_str(),
                                              &ftp_opattr,
                                              &ftp_complete_callback,
                                              cbarg));
    if (!res) {
      logger.msg(VERBOSE, "Rename: globus_ftp_client_move failed");
      std::string err(res.str());
      logger.msg(VERBOSE, "Globus error: %s", err);
      return DataStatus(DataStatus::RenameError, err);
    }
    if (!cond.wait(1000*usercfg.Timeout())) {
      logger.msg(VERBOSE, "Rename: timeout waiting for operation to complete");
      GlobusResult(globus_ftp_client_abort(&ftp_handle));
      cond.wait();
      return DataStatus(DataStatus::RenameError, EARCREQUESTTIMEOUT, "Timeout waiting for rename at "+url.plainstr());
    }
    if (!callback_status) {
      return DataStatus(DataStatus::RenameError, callback_status.GetErrno(), callback_status.GetDesc());
    }
    return DataStatus::Success;
  }

  DataPointGridFTP::DataPointGridFTP(const URL& url, const UserConfig& usercfg, PluginArgument* parg)
    : DataPointDirect(url, usercfg, parg),
      cbarg(new CBArg(this)),
      ftp_active(false),
      credential(NULL),
      reading(false),
      writing(false),
      ftp_eof_flag(false),
      check_received_length(0),
      data_error(false),
      lister(NULL) {
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
      GlobusResult(globus_thread_set_model("pthread"));
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
        GlobusResult(globus_ftp_client_handleattr_destroy(&ftp_attr));
        logger.msg(ERROR, "init_handle: "
                   "globus_ftp_client_handleattr_set_gridftp2 failed");
        logger.msg(ERROR, "Globus error: %s", res.str());
        ftp_active = false;
        return;
      }
#endif
      if (!(res = globus_ftp_client_handle_init(&ftp_handle, &ftp_attr))) {
        GlobusResult(globus_ftp_client_handleattr_destroy(&ftp_attr));
        logger.msg(ERROR, "init_handle: globus_ftp_client_handle_init failed");
        logger.msg(ERROR, "Globus error: %s", res.str());
        ftp_active = false;
        return;
      }
      GlobusResult(globus_ftp_client_handleattr_destroy(&ftp_attr));
      if (!(res = globus_ftp_client_operationattr_init(&ftp_opattr))) {
        logger.msg(ERROR, "init_handle: "
                   "globus_ftp_client_operationattr_init failed");
        logger.msg(ERROR, "Globus error: %s", res.str());
        GlobusResult(globus_ftp_client_handle_destroy(&ftp_handle));
        ftp_active = false;
        return;
      }
      if (!(res = globus_ftp_client_operationattr_set_allow_ipv6(&ftp_opattr, GLOBUS_TRUE))) {
        logger.msg(WARNING, "init_handle: "
                   "globus_ftp_client_operationattr_set_allow_ipv6 failed");
        logger.msg(WARNING, "Globus error: %s", res.str());
      }
      if (!(res = globus_ftp_client_operationattr_set_delayed_pasv(&ftp_opattr,
                                                                   GLOBUS_TRUE))) {
        logger.msg(WARNING, "init_handle: "
                   "globus_ftp_client_operationattr_set_delayed_pasv failed");
        logger.msg(WARNING, "Globus error: %s", res.str());
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
    lister = new Lister();
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
    GlobusResult(globus_ftp_client_operationattr_set_parallelism(&ftp_opattr, &paral));
    GlobusResult(globus_ftp_client_operationattr_set_striped(&ftp_opattr, GLOBUS_FALSE));
    /*   globus_ftp_client_operationattr_set_layout         */
    /*   globus_ftp_client_operationattr_set_tcp_buffer     */
    GlobusResult(globus_ftp_client_operationattr_set_type(&ftp_opattr,
                                             GLOBUS_FTP_CONTROL_TYPE_IMAGE));
    if (!is_secure) { // plain ftp protocol
      GlobusResult r(globus_ftp_client_operationattr_set_authorization(
                     &ftp_opattr,
                     GSS_C_NO_CREDENTIAL,
                     url.Username().empty() ? "anonymous" : url.Username().c_str(),
                     url.Passwd().empty() ? NULL : url.Passwd().c_str(),
                     GLOBUS_NULL, GLOBUS_NULL));
      if(!r) {
        logger.msg(VERBOSE, "globus_ftp_client_operationattr_set_authorization: error: %s", r.str());
      }

      GlobusResult(globus_ftp_client_operationattr_set_mode(&ftp_opattr,
                                               GLOBUS_FTP_CONTROL_MODE_STREAM));
      GlobusResult(globus_ftp_client_operationattr_set_data_protection(&ftp_opattr,
                                                          GLOBUS_FTP_CONTROL_PROTECTION_CLEAR));
      GlobusResult(globus_ftp_client_operationattr_set_control_protection(&ftp_opattr,
                                                             GLOBUS_FTP_CONTROL_PROTECTION_CLEAR));
      // need to set dcau to none in order Globus libraries not to send
      // it to pure ftp server
      globus_ftp_control_dcau_t dcau;
      dcau.mode = GLOBUS_FTP_CONTROL_DCAU_NONE;
      GlobusResult(globus_ftp_client_operationattr_set_dcau(&ftp_opattr, &dcau));
    }
    else { // gridftp protocol

      if (!credential){
        credential = new GSSCredential(usercfg);
      }
      lister->set_credential(credential);

      GlobusResult r(globus_ftp_client_operationattr_set_authorization(
                     &ftp_opattr,
                     *credential,":globus-mapping:","user@",
                     GLOBUS_NULL,GLOBUS_NULL));
      if(!r) {
        logger.msg(WARNING, "Failed to set credentials for GridFTP transfer");
        logger.msg(VERBOSE, "globus_ftp_client_operationattr_set_authorization: error: %s", r.str());
      }
      if (force_secure || (url.Option("secure") == "yes")) {
        GlobusResult(globus_ftp_client_operationattr_set_data_protection(&ftp_opattr,
                                    GLOBUS_FTP_CONTROL_PROTECTION_PRIVATE));
        logger.msg(VERBOSE, "Using secure data transfer");
      }
      else {
        GlobusResult(globus_ftp_client_operationattr_set_data_protection(&ftp_opattr,
                                    GLOBUS_FTP_CONTROL_PROTECTION_CLEAR));
        logger.msg(VERBOSE, "Using insecure data transfer");

        globus_ftp_control_dcau_t dcau;
        dcau.mode = GLOBUS_FTP_CONTROL_DCAU_NONE;
        GlobusResult(globus_ftp_client_operationattr_set_dcau(&ftp_opattr, &dcau));
      }
      if (force_passive) {
        GlobusResult(globus_ftp_client_operationattr_set_mode(&ftp_opattr,
                             GLOBUS_FTP_CONTROL_MODE_STREAM));
      } else {
        GlobusResult(globus_ftp_client_operationattr_set_mode(&ftp_opattr,
                             GLOBUS_FTP_CONTROL_MODE_EXTENDED_BLOCK));
      }
      GlobusResult(globus_ftp_client_operationattr_set_control_protection(&ftp_opattr,
                                     GLOBUS_FTP_CONTROL_PROTECTION_PRIVATE));
    }
    /*   globus_ftp_client_operationattr_set_dcau                         */
    /*   globus_ftp_client_operationattr_set_resume_third_party_transfer  */
    /*   globus_ftp_client_operationattr_set_authorization                */
    GlobusResult(globus_ftp_client_operationattr_set_append(&ftp_opattr, GLOBUS_FALSE));
  }

  DataPointGridFTP::~DataPointGridFTP() {
    int destroy_timeout = 15+1; // waiting some reasonable time for globus
    StopReading();
    StopWriting();
    if (ftp_active) {
      logger.msg(DEBUG, "~DataPoint: destroy ftp_handle");
      // In case globus is still doing something asynchronously
      while(!GlobusResult(globus_ftp_client_handle_destroy(&ftp_handle))) {
        logger.msg(VERBOSE, "~DataPoint: destroy ftp_handle failed - retrying");
        if(!(--destroy_timeout)) break;
        // Unfortunately there is no sutable condition to wait for.
        // But such situation should happen very rarely if ever. I hope so.
        // It is also expected Globus will call all pending callbacks here 
        // so it is free to destroy DataPointGridFTP and related objects.
        sleep(1);
      }
      if(destroy_timeout) GlobusResult(globus_ftp_client_operationattr_destroy(&ftp_opattr));
    }
    if (credential) delete credential;
    if (lister) delete lister;
    cbarg->abandon(); // acquires lock
    if(destroy_timeout) {
      delete cbarg;
    } else {
      // So globus maybe did not call all callbacks. Keeping 
      // intermediate object.
      logger.msg(VERBOSE, "~DataPoint: failed to destroy ftp_handle - leaking");
    }
    // Clean all Globus error objects which Globus forgot to properly process.
    GlobusResult::wipe();
    // See activation for description
    //GlobusResult(globus_module_deactivate(GLOBUS_FTP_CLIENT_MODULE));
  }

  Plugin* DataPointGridFTP::Instance(PluginArgument *arg) {
    DataPointPluginArgument *dmcarg = dynamic_cast<DataPointPluginArgument*>(arg);
    if (!dmcarg) return NULL;
    if (((const URL&)(*dmcarg)).Protocol() != "gsiftp" &&
        ((const URL&)(*dmcarg)).Protocol() != "ftp") {
      return NULL;
    }
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
    return new DataPointGridFTP(*dmcarg, *dmcarg, dmcarg);
  }

  bool DataPointGridFTP::WriteOutOfOrder() const {
    return true;
  }

  bool DataPointGridFTP::ProvidesMeta() const {
    return true;
  }

  const std::string DataPointGridFTP::DefaultCheckSum() const {
    // no way to know which checksum is used for each file, so hard-code adler32 for now
    return std::string("adler32");
  }

  bool DataPointGridFTP::RequiresCredentials() const {
    return is_secure;
  }

  bool DataPointGridFTP::SetURL(const URL& u) {
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
    // Cache control connection
    GlobusResult(globus_ftp_client_handle_cache_url_state(&ftp_handle, url.plainstr().c_str()));
    return true;
  }

  DataPointGridFTP* DataPointGridFTP::CBArg::acquire(void) {
    lock.lock();
    if(!arg) {
      lock.unlock();
    }
    return arg;
  }

  void DataPointGridFTP::CBArg::release(void) {
    lock.unlock();
  }

  DataPointGridFTP::CBArg::CBArg(DataPointGridFTP* a) {
    arg = a;
  }

  void DataPointGridFTP::CBArg::abandon(void) {
    lock.lock();
    arg = NULL;
    lock.unlock();
  }

} // namespace ArcDMCGridFTP

extern Arc::PluginDescriptor const ARC_PLUGINS_TABLE_NAME[] = {
  { "gsiftp", "HED:DMC.disabled", "FTP or FTP with GSI security", 0, &ArcDMCGridFTP::DataPointGridFTP::Instance },
  { NULL, NULL, NULL, 0, NULL }
};

extern "C" {
  void ARC_MODULE_CONSTRUCTOR_NAME(Glib::Module* module, Arc::ModuleManager* manager) {
    if(manager && module) {
      manager->makePersistent(module);
    };
  }
}

