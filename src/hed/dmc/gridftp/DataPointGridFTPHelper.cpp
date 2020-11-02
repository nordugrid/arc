// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <openssl/ssl.h>

#include <arc/Logger.h>
#include <arc/ArcLocation.h>
#include <arc/OptionParser.h>
#include <arc/StringConv.h>
#include <arc/UserConfig.h>
//#include <arc/data/DataBuffer.h>
#include <arc/data/DataPointDirect.h>
#include <arc/CheckSum.h>
#include <arc/globusutils/GlobusErrorUtils.h>
#include <arc/globusutils/GlobusWorkarounds.h>
#include <arc/globusutils/GSSCredential.h>
#include <arc/crypto/OpenSSL.h>
#include <arc/data/DataExternalComm.h>
#include <arc/data/DataPointDelegate.h>

#include "Lister.h"

#include "DataPointGridFTPHelper.h"

namespace ArcDMCGridFTP {

  using namespace Arc;

  static bool proxy_initialized = false;

  char dummy_buffer = 0;

  Logger DataPointGridFTPHelper::logger(Logger::getRootLogger(), "DataPoint.GridFTP");

  static std::string const default_checksum("adler32");

  class ChunkOffsetMatch {
   public:
    ChunkOffsetMatch(unsigned long long int offset):offset(offset) {};
    bool operator()(DataExternalComm::DataChunkClient const& other) const { return offset == other.getOffset(); };
   private:
    unsigned long long int offset; 
  };

  void DataPointGridFTPHelper::ftp_complete_callback(void *arg,
                                               globus_ftp_client_handle_t*,
                                               globus_object_t *error) {
    DataPointGridFTPHelper *it = ((CBArg*)arg)->acquire();
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

  void DataPointGridFTPHelper::ftp_check_callback(void *arg,
                                            globus_ftp_client_handle_t*,
                                            globus_object_t *error,
                                            globus_byte_t*,
                                            globus_size_t length,
                                            globus_off_t,
                                            globus_bool_t eof) {
    DataPointGridFTPHelper *it = ((CBArg*)arg)->acquire();
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

  DataStatus DataPointGridFTPHelper::Check() {
    if (!ftp_active)
      return DataStatus::NotInitializedError;
    GlobusResult res;
    bool size_obtained = false;
    set_attributes();
    // check if file or directory - can't do a get on a directory
    if (!lister->retrieve_file_info(url,true)) return DataStatus::CheckError;
    if (lister->size() == 0) return DataStatus::CheckError;
    if (lister->size() != 1) {
      // guess - that probably means it is directory 
      // successful stat is enough to report successful access to a directory
      return DataStatus::Success;
    } else {
      FileInfo lister_info(*(lister->begin()));
      if(lister_info.GetType() != FileInfo::file_type_file)
        // successful stat is enough to report successful access to a directory
        return DataStatus::Success;
      if(lister_info.GetSize() != ((unsigned long long int)(-1)))
        size_obtained = true;
    }

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
      cond.reset();
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
      if (!size_obtained) return DataStatus::CheckError;
      return DataStatus::Success;
    }
  }

  DataStatus DataPointGridFTPHelper::Remove() {
    if (!ftp_active) return DataStatus::NotInitializedError;
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

  DataStatus DataPointGridFTPHelper::RemoveFile() {
    cond.reset();
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

  DataStatus DataPointGridFTPHelper::RemoveDir() {
    cond.reset();
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

  bool DataPointGridFTPHelper::mkdir_ftp() {
    std::string ftp_dir_path = url.plainstr();
    for (;;)
      if (!remove_last_dir(ftp_dir_path))
        break;
    bool result = true;
    for (;;) {
      if (!add_last_dir(ftp_dir_path, url.plainstr()))
        break;
      logger.msg(VERBOSE, "mkdir_ftp: making %s", ftp_dir_path);
      cond.reset();
      GlobusResult res(globus_ftp_client_mkdir(&ftp_handle, ftp_dir_path.c_str(), &ftp_opattr,
                                &ftp_complete_callback, cbarg));
      if (!res) {
        logger.msg(INFO, "Globus error: %s", res.str());
        return false;
      }
      if (!cond.wait(1000*usercfg.Timeout())) {
        logger.msg(INFO, "mkdir_ftp: timeout waiting for mkdir");
        // timeout - have to cancel operation here 
        GlobusResult(globus_ftp_client_abort(&ftp_handle));
        cond.wait();
        return false;
      }
      if (!callback_status)
        result = false;
    }
    return result;
  }

  DataStatus DataPointGridFTPHelper::CreateDirectory(bool with_parents) {
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
    cond.reset();
    GlobusResult res(globus_ftp_client_mkdir(&ftp_handle, dirpath.c_str(), &ftp_opattr,
                              &ftp_complete_callback, cbarg));
    if (!res) {
      std::string err(res.str());
      logger.msg(VERBOSE, "Globus error: %s", err);
      return DataStatus(DataStatus::CreateDirectoryError, err);
    }
    if (!cond.wait(1000*usercfg.Timeout())) {
      logger.msg(VERBOSE, "Timeout waiting for mkdir");
      // timeout - have to cancel operation here
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

  DataStatus DataPointGridFTPHelper::Read() {
    if (!ftp_active) return DataStatus::NotInitializedError;
    set_attributes();
    delayed_chunks.clear();
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
      return DataStatus(DataStatus::ReadStartError, globus_err);
    }
    // Prepare and inject buffers
    max_offset = 0;
    int n_buffers = 0;
    for(int n = 0; n < (ftp_threads*2);++n) {
      globus_byte_t* buffer = new globus_byte_t[ftp_bufsize];
      data_counter.inc();
      GlobusResult res(globus_ftp_client_register_read(&ftp_handle, buffer, ftp_bufsize, &ftp_read_callback, cbarg));
      if(!res) {
        data_counter.dec();
        delete[] buffer;
        logger.msg(DEBUG, "ftp_read_thread: Globus error: %s", res.str());
        continue;
      }
      ++n_buffers;
    }
    bool must_detach = false;
    if(n_buffers <= 0) {
      // No buffer was registered - must be something bad with registring buffers into globus
      logger.msg(VERBOSE, "ftp_read_thread: failed to register buffers");
      GlobusResult res(globus_ftp_client_abort(&ftp_handle));
      if(!res) must_detach = true;
      callback_status = DataStatus(DataStatus::ReadStartError);
    } else {
      // make sure globus has thread for handling network/callbacks
      GlobusResult(globus_thread_blocking_will_block());
      // make sure complete callback is called
      logger.msg(VERBOSE, "ftp_read_thread: waiting for eof");
      cond.wait();
      // And now make sure all buffers were released in case Globus calls
      // complete_callback before calling all read_callbacks
      logger.msg(VERBOSE, "ftp_read_thread: waiting for buffers released");
      if(!data_counter.wait(15)) {
        // See comment in ftp_write_thread for explanation.
        logger.msg(VERBOSE, "ftp_read_thread: failed to release buffers");
        must_detach = true;
      }
    }
    if(must_detach) {
      logger.msg(VERBOSE, "ftp_read_thread: failed to release buffers - leaking");
      // Detach globus 
      CBArg* cbarg_old = cbarg;
      cbarg = new CBArg(this);
      cbarg_old->abandon();
    };
    // TODO: wait for ftp_put_complete_callback
    logger.msg(VERBOSE, "ftp_read_thread: exiting");
    return callback_status;
  }

  void DataPointGridFTPHelper::ftp_read_callback(void *arg,
                                           globus_ftp_client_handle_t*,
                                           globus_object_t *error,
                                           globus_byte_t *buffer,
                                           globus_size_t length,
                                           globus_off_t offset,
                                           globus_bool_t eof) {
    DataPointGridFTPHelper *it = ((CBArg*)arg)->acquire();
    if(!it) {
      // It was detached
      delete[] buffer;
      return;
    }
    std::ostream& outstream(it->outstream);
    if (error != GLOBUS_SUCCESS) {
      // Report error and release buffer
      it->data_error = true;
      logger.msg(VERBOSE, "ftp_read_callback: failure: %s",globus_object_to_string(error));
      delete[] buffer;
      it->data_counter.dec();
      it->data_counter_change.signal();
    } else {
      logger.msg(DEBUG, "ftp_read_callback: success - offset=%u, length=%u, eof=%u, allow oof=%u",(int)offset,(int)length,(int)eof,(int)it->allow_out_of_order);
      if(length > 0) {
        // Report received content
        DataExternalComm::DataChunkClient dataChunk(buffer, offset, length);
        if(it->allow_out_of_order || (offset == it->max_offset)) {
          dataChunk.write(outstream<<DataExternalComm::DataChunkTag);
          it->max_offset = offset+length;
          // Check for delayed buffers which fit new offset
          while(!it->delayed_chunks.empty()) {
            DataExternalComm::DataChunkClientList::iterator matching_chunk =
              std::find_if(it->delayed_chunks.begin(), it->delayed_chunks.end(), ChunkOffsetMatch(it->max_offset));
            if(matching_chunk == it->delayed_chunks.end()) break;
            unsigned long long int offset = matching_chunk->getOffset();
            unsigned long long int length = matching_chunk->getSize();
            logger.msg(VERBOSE, "ftp_read_callback: delayed data chunk: %llu %llu", offset, length);
            matching_chunk->write(outstream<<DataExternalComm::DataChunkTag);
            it->delayed_chunks.erase(matching_chunk);
            it->max_offset = offset+length;
          }
        } else {
          // protection against data coming out of order - receiving end can't handle that
          logger.msg(WARNING, "ftp_read_callback: unexpected data out of order: %llu != %llu", offset, it->max_offset);
          // The numbers below are just wild guess.
          if((it->delayed_chunks.size() < (it->ftp_threads*10)) || ((offset - it->max_offset) < 1024*1024*256)) {
            it->delayed_chunks.push_back(dataChunk.MakeCopy()); // move asignment passes ownership
          } else {
            // Can't use data from this buffer - drop it ad report error
            it->data_error = true;
            logger.msg(ERROR, "ftp_read_callback: too many unexpected out of order chunks");
            delete[] buffer;
            it->data_counter.dec();
            it->data_counter_change.signal();
          }
        }
      }
      if (eof) it->ftp_eof_flag = true;
      if (it->ftp_eof_flag) {
        // Transmission is over - do not register buffer again
        delete[] buffer;
        it->data_counter.dec();
        it->data_counter_change.signal();
      } else {
        // Re-register buffer
        // TODO: Check if callback can be called inside globus_ftp_client_register_read
        GlobusResult res(globus_ftp_client_register_read(&it->ftp_handle, buffer, it->ftp_bufsize, &ftp_read_callback, arg));
        if(!res) {
          delete[] buffer;
          it->data_counter.dec();
          it->data_counter_change.signal();
          logger.msg(DEBUG, "ftp_read_callback: Globus error: %s", res.str());
        }
      }
    }

    if(it->data_counter.get() == 0) {
      DataExternalComm::DataChunkClient dataEntry(NULL, offset+length, 0); // using 0 size as eof indication
      dataEntry.write(outstream<<DataExternalComm::DataChunkTag);
      if(!it->ftp_eof_flag) {
        // Must be case when all buffers are gone due to errors
        GlobusResult(globus_ftp_client_abort(&it->ftp_handle));
        // Not checking for result because nothing can be done anyway
      }
    }
    ((CBArg*)arg)->release();
    return;
  }

  void DataPointGridFTPHelper::ftp_get_complete_callback(void *arg,
                                                   globus_ftp_client_handle_t*,
                                                   globus_object_t *error) {
    DataPointGridFTPHelper *it = ((CBArg*)arg)->acquire();
    if(!it) return;
    // data transfer finished
    if (error != GLOBUS_SUCCESS) {
      logger.msg(INFO, "ftp_get_complete_callback: Failed to get ftp file");
      std::string err(trim(globus_object_to_string(error)));
      logger.msg(VERBOSE, "%s", err);
      it->callback_status = DataStatus(DataStatus::ReadStartError, globus_error_to_errno(err, EARCOTHER), err);
    } else {
      logger.msg(DEBUG, "ftp_get_complete_callback: success");
      it->callback_status = DataStatus::Success;
    }
    it->cond.signal();
    ((CBArg*)arg)->release();
    return;
  }

  DataStatus DataPointGridFTPHelper::Write() {
    if (!ftp_active) return DataStatus::NotInitializedError;
    set_attributes();
    delayed_chunks.clear();
    // size of file first
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
        logger.msg(VERBOSE, "start_writing_ftp: mkdir failed - still trying to write");
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
      return DataStatus(DataStatus::WriteStartError, globus_err);
    }
    // make sure globus has thread for handling network/callbacks
    GlobusResult(globus_thread_blocking_will_block());
    // Read and inject buffers
    data_error = false;
    data_counter.set(0);
    max_offset = 0;
    while(true) {
      static globus_byte_t dummy;
      logger.msg(VERBOSE, "start_writing_ftp: waiting for data tag");
      char c = DataExternalComm::InTag(instream);
      if(c != DataExternalComm::DataChunkTag) {
        logger.msg(ERROR, "start_writing_ftp: failed to read data tag");
        GlobusResult(globus_ftp_client_abort(&ftp_handle));
        break;
      }
      DataExternalComm::DataChunkClient dataChunk;
      logger.msg(VERBOSE, "start_writing_ftp: waiting for data chunk");
      if(!dataChunk.read(instream)) {
        logger.msg(ERROR, "start_writing_ftp: failed to read data chunk");
        GlobusResult(globus_ftp_client_abort(&ftp_handle));
        break;
      }
      unsigned long long int offset = dataChunk.getOffset();
      unsigned long long int length = dataChunk.getSize();
      globus_byte_t* data = reinterpret_cast<globus_byte_t*>(dataChunk.get());
      if(length == 0) data = &dummy;
      if(stream_mode) {
        // protection against data coming out of order - stream mode can't handle that
        if(offset != max_offset) {
          logger.msg(WARNING, "ftp_write_thread: data out of order in stream mode: %llu != %llu", offset, max_offset);
          // The numbers below are just wild guess. It i snot even proper place to compensate for out
          // of order data chunks. It should have been fixed on reading side.
          if((delayed_chunks.size() < (ftp_threads*10)) || ((offset-max_offset) < 1024*1024*256)) {
            delayed_chunks.push_back(dataChunk); // move asignment passes ownership
            continue;
          }
          logger.msg(ERROR, "ftp_write_thread: too many out of order chunks in stream mode");
          GlobusResult(globus_ftp_client_abort(&ftp_handle));
          break;
        }
      }
      logger.msg(VERBOSE, "start_writing_ftp: data chunk: %llu %llu", offset, length);
      data_counter.inc();
      GlobusResult res(globus_ftp_client_register_write(&ftp_handle, data, length, offset,
                                           dataChunk.getEOF()?GLOBUS_TRUE:GLOBUS_FALSE, &ftp_write_callback, cbarg));
      if(!res) {
        data_counter.dec();
        logger.msg(DEBUG, "ftp_write_thread: Globus error: %s", res.str());
        GlobusResult(globus_ftp_client_abort(&ftp_handle));
        break;
      }
      if(length != 0) dataChunk.release(); // pass ownership to globus
      if((offset+length) > max_offset) max_offset = offset+length; 

      // Do we have any delayed buffers we could release?
      while(!delayed_chunks.empty()) {
        DataExternalComm::DataChunkClientList::iterator matching_chunk =
          std::find_if(delayed_chunks.begin(), delayed_chunks.end(), ChunkOffsetMatch(max_offset));
        if(matching_chunk == delayed_chunks.end()) break;
        dataChunk = *matching_chunk;
        delayed_chunks.erase(matching_chunk);

        unsigned long long int offset = dataChunk.getOffset();
        unsigned long long int length = dataChunk.getSize();
        globus_byte_t* data = reinterpret_cast<globus_byte_t*>(dataChunk.get());
        if(length == 0) data = &dummy;
        logger.msg(VERBOSE, "start_writing_ftp: delayed data chunk: %llu %llu", offset, length);
        data_counter.inc();
        res = globus_ftp_client_register_write(&ftp_handle,
                                             data, length, offset,
                                             dataChunk.getEOF()?GLOBUS_TRUE:GLOBUS_FALSE, &ftp_write_callback, cbarg);
        if(!res) {
          data_counter.dec();
          logger.msg(DEBUG, "ftp_write_thread: Globus error: %s", res.str());
          GlobusResult(globus_ftp_client_abort(&ftp_handle));
          break;
        }
        if(length != 0) dataChunk.release(); // pass ownership to globus
        max_offset = offset+length; 
      }
      if(!res) break;
      
      // Prevent pile up of buffers in memory
      while(data_counter.get() >= (ftp_threads*2)) {
        // Wait for some buffers to be released
        logger.msg(VERBOSE, "start_writing_ftp: waiting for some buffers sent");
        data_counter_change.wait();
      }
      if(dataChunk.getEOF()) break; // no more chunks
    }
    // Waiting for data transfer to finish
    logger.msg(VERBOSE, "ftp_write_thread: waiting for transfer complete");
    cond.wait();
    logger.msg(VERBOSE, "ftp_write_thread: waiting for buffers released");
    if(!data_counter.wait(15)) {
      logger.msg(VERBOSE, "ftp_read_thread: failed to release buffers - leaking");
      CBArg* cbarg_old = cbarg;
      cbarg = new CBArg(this);
      cbarg_old->abandon();
    };
    logger.msg(VERBOSE, "ftp_write_thread: exiting");
    return callback_status;
  }

  void DataPointGridFTPHelper::ftp_write_callback(void *arg,
                                            globus_ftp_client_handle_t*,
                                            globus_object_t *error,
                                            globus_byte_t *buffer,
                                            globus_size_t length,
                                            globus_off_t offset,
                                            globus_bool_t is_eof) {
    DataPointGridFTPHelper *it = ((CBArg*)arg)->acquire();
    if(!it) {
      // detached
      if(length != 0) delete[] buffer;
      return;
    }
    if (error != GLOBUS_SUCCESS) {
      it->data_error = true;
      logger.msg(VERBOSE, "ftp_write_callback: failure: %s",globus_object_to_string(error));
    } else {
      logger.msg(DEBUG, "ftp_write_callback: success %s",is_eof?"eof":"   ");
    }
    it->data_counter.dec();
    it->data_counter_change.signal();
    if(length != 0) delete[] buffer;
    ((CBArg*)arg)->release();
    return;
  }

  void DataPointGridFTPHelper::ftp_put_complete_callback(void *arg,
                                                   globus_ftp_client_handle_t*,
                                                   globus_object_t *error) {
    DataPointGridFTPHelper *it = ((CBArg*)arg)->acquire();
    if(!it) return;
    // data transfer finished
    if (error != GLOBUS_SUCCESS) {
      logger.msg(INFO, "Failed to store ftp file");
      std::string err(trim(globus_object_to_string(error)));
      logger.msg(VERBOSE, "%s", err);
      it->callback_status = DataStatus(DataStatus::WriteStartError, globus_error_to_errno(err, EARCOTHER), err);
    } else {
      logger.msg(DEBUG, "ftp_put_complete_callback: success");
      it->callback_status = DataStatus::Success;
    }
    it->cond.signal();
    ((CBArg*)arg)->release();
    return;
  }

  DataStatus DataPointGridFTPHelper::do_more_stat(FileInfo& f, DataPoint::DataPointInfoType verb) {
    DataStatus result = DataStatus::Success;
    GlobusResult res;
    globus_off_t size = 0;
    std::string f_url = url.ConnectionURL() + f.GetName();
    if (((verb & DataPoint::INFO_TYPE_CONTENT) == DataPoint::INFO_TYPE_CONTENT) && (!f.CheckSize()) && (f.GetType() != FileInfo::file_type_dir)) {
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
    if ((verb & DataPoint::INFO_TYPE_TIMES) == DataPoint::INFO_TYPE_TIMES && !f.CheckModified()) {
      globus_abstime_t gl_modify_time;
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
        time_t modify_time;
        int modify_utime;
        GlobusTimeAbstimeGet(gl_modify_time, modify_time, modify_utime);
        f.SetModified(modify_time);
        modify_utime = modify_utime;
      }
    }
    if ((verb & DataPoint::INFO_TYPE_CKSUM) == DataPoint::INFO_TYPE_CKSUM && !f.CheckCheckSum() && f.GetType() != FileInfo::file_type_dir) {
      // not all implementations support checksum so failure is not an error
      logger.msg(DEBUG, "list_files_ftp: "
                        "looking for checksum of %s", f_url);
      char cksum[256];
      std::string cksumtype(upper(default_checksum).c_str());
      cond.reset();
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
        if (callback_status == EOPNOTSUPP)
          logger.msg(INFO, "list_files_ftp: no checksum information supported");
        else 
          logger.msg(INFO, "list_files_ftp: no checksum information returned");
        callback_status = DataStatus::Success;
      }
      else {
        logger.msg(VERBOSE, "list_files_ftp: checksum %s", cksum);
        f.SetCheckSum(default_checksum + ':' + std::string(cksum));
      }
    }
    return result;
  }

  DataStatus DataPointGridFTPHelper::Stat(DataPoint::DataPointInfoType verb) {
    if (!ftp_active) return DataStatus::NotInitializedError;
    set_attributes();
    bool more_info = ((verb | DataPoint::INFO_TYPE_NAME) != DataPoint::INFO_TYPE_NAME);
    DataStatus lister_res = lister->retrieve_file_info(url,!more_info);
    if (!lister_res) {
      logger.msg(VERBOSE, "Failed to obtain stat from FTP: %s", lister_res.GetDesc());
      return lister_res;
    }
    DataStatus result = DataStatus::StatError;
    if (lister->size() == 0) {
      logger.msg(VERBOSE, "No results returned from stat");
      result.SetDesc("No results found for "+url.plainstr());
      return result;
    }
    FileInfo file;
    if(lister->size() != 1) {
      logger.msg(VERBOSE, "Wrong number of objects (%i) for stat from ftp: %s", lister->size(), url.plainstr());
      // guess - that probably means it is directory 
      file.SetName(FileInfo(url.Path()).GetName());
      file.SetType(FileInfo::file_type_dir);
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
    }
    if (lister_info.CheckModified()) {
      file.SetModified(lister_info.GetModified());
    }
    if (lister_info.CheckCheckSum()) {
      file.SetCheckSum(lister_info.GetCheckSum());
    }
    if(result)
      DataExternalComm::OutEntry(outstream<<DataExternalComm::FileInfoTag, file);
    return result;
  }

  DataStatus DataPointGridFTPHelper::List(DataPoint::DataPointInfoType verb) {
    if (!ftp_active) return DataStatus::NotInitializedError;
    set_attributes();
    bool more_info = ((verb | DataPoint::INFO_TYPE_NAME) != DataPoint::INFO_TYPE_NAME);
    DataStatus lister_res = lister->retrieve_dir_info(url,!more_info);
    if (!lister_res) {
      logger.msg(VERBOSE, "Failed to obtain listing from FTP: %s", lister_res.GetDesc());
      return lister_res;
    }
    DataStatus result = DataStatus::Success;
    int cksum_failed_cnt = 0;
    for (std::list<FileInfo>::iterator i = lister->begin();
         i != lister->end(); ++i) {
      if (i->GetName()[0] != '/') i->SetName(url.Path()+'/'+i->GetName());
      if (more_info) {
        DataStatus r = do_more_stat(*i, verb);
        if(!r) {
          if(r == DataStatus::StatError) r = DataStatus(DataStatus::ListError, r.GetDesc());
          result = r;
        }
      }
      DataExternalComm::OutEntry(outstream<<DataExternalComm::FileInfoTag, *i);
      if(((verb & DataPoint::INFO_TYPE_CKSUM) == DataPoint::INFO_TYPE_CKSUM) &&
         (i->GetType() != FileInfo::file_type_dir)) {
        if(i->GetCheckSum().empty()) {
          if(++cksum_failed_cnt >= 10) {
            verb = (Arc::DataPoint::DataPointInfoType)(verb & ~DataPoint::INFO_TYPE_CKSUM);
            logger.msg(VERBOSE, "Too many failures to obtain checksum - giving up");
          }
        } else {
          cksum_failed_cnt = 0;
        }
      }
    }
    return result;
  }

  DataStatus DataPointGridFTPHelper::Rename(const URL& newurl) {
    if (!ftp_active)
      return DataStatus::NotInitializedError;
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

  DataPointGridFTPHelper::DataPointGridFTPHelper(const URL& url, const UserConfig& usercfg, std::istream& instream, std::ostream& outstream):
      ftp_active(false),
      url(url),
      usercfg(usercfg),
      instream(instream),
      outstream(outstream),
      cbarg(new CBArg(this)),
      force_secure(true),
      force_passive(true),
      ftp_threads(1),
      range_start(0),
      range_end(0),
      allow_out_of_order(true),
      stream_mode(true),
      credential(NULL),
      ftp_eof_flag(false),
      check_received_length(0),
      data_error(false),
      lister(NULL) {
    if (url.Protocol() == "gsiftp") {
      is_secure = true;
    } else if(url.Protocol() == "ftp") {
      is_secure = false;
    } else {
      return;
    };
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
    autodir = true;
    std::string autodir_s = url.Option("autodir");
    if(autodir_s == "yes") {
      autodir = true;
    } else if(autodir_s == "no") {
      autodir = false;
    }
    lister = new Lister();
  }

  void DataPointGridFTPHelper::set_attributes(void) {
    ftp_threads = 1;
    ftp_bufsize = 65536;
    if (allow_out_of_order) {
      if(stringto<int>(url.Option("threads"), ftp_threads)) {
        if (ftp_threads < 1)
          ftp_threads = 1;
        if (ftp_threads > MAX_PARALLEL_STREAMS)
          ftp_threads = MAX_PARALLEL_STREAMS;
      } else {
        ftp_threads = 1;
      }
    }
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

      stream_mode = true;
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
        stream_mode = true;
        GlobusResult(globus_ftp_client_operationattr_set_mode(&ftp_opattr,
                             GLOBUS_FTP_CONTROL_MODE_STREAM));
      } else {
        stream_mode = false;
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

  DataPointGridFTPHelper::~DataPointGridFTPHelper() {
    int destroy_timeout = 15+1; // waiting some reasonable time for globus
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

  DataPointGridFTPHelper* DataPointGridFTPHelper::CBArg::acquire(void) {
    lock.lock();
    if(!arg) {
      lock.unlock();
    }
    return arg;
  }

  void DataPointGridFTPHelper::CBArg::release(void) {
    lock.unlock();
  }

  DataPointGridFTPHelper::CBArg::CBArg(DataPointGridFTPHelper* a) {
    arg = a;
  }

  void DataPointGridFTPHelper::CBArg::abandon(void) {
    lock.lock();
    arg = NULL;
    lock.unlock();
  }

} // namespace ArcDMCGridFTP

int main(int argc, char* argv[]) {
  using namespace Arc;
  // Ignore some signals
  signal(SIGTTOU,SIG_IGN);
  signal(SIGPIPE,SIG_IGN);
  signal(SIGHUP,SIG_IGN);
  // Debug code for setting different logging formats
  char const * log_time_format = ::getenv("ARC_LOGGER_TIME_FORMAT");
  if(log_time_format) {
    if(strcmp(log_time_format,"USER") == 0) {
      Arc::Time::SetFormat(Arc::UserTime);
    } else if(strcmp(log_time_format,"USEREXT") == 0) {
      Arc::Time::SetFormat(Arc::UserExtTime);
    } else if(strcmp(log_time_format,"ELASTIC") == 0) {
      Arc::Time::SetFormat(Arc::ElasticTime);
    } else if(strcmp(log_time_format,"MDS") == 0) {
      Arc::Time::SetFormat(Arc::MDSTime);
    } else if(strcmp(log_time_format,"ASC") == 0) {
      Arc::Time::SetFormat(Arc::ASCTime);
    } else if(strcmp(log_time_format,"ISO") == 0) {
      Arc::Time::SetFormat(Arc::ISOTime);
    } else if(strcmp(log_time_format,"UTC") == 0) {
      Arc::Time::SetFormat(Arc::UTCTime);
    } else if(strcmp(log_time_format,"RFC1123") == 0) {
      Arc::Time::SetFormat(Arc::RFC1123Time);
    } else if(strcmp(log_time_format,"EPOCH") == 0) {
      Arc::Time::SetFormat(Arc::EpochTime);
    };
  };
  // Temporary stderr destination for error messages
  Arc::LogStream logcerr(std::cerr);
  Arc::Logger::getRootLogger().addDestination(logcerr);
  if((argc>0) && (argv[0])) Arc::ArcLocation::Init(argv[0]);
  std::list<std::string> params;
  int bufsize = 1024*1024;
  int range_start = 0;
  int range_end = 0;
  std::string logger_verbosity;
  int logger_format = -1;
  int secure = 1;
  int passive = 1;
  int out_of_order = 1;

  try {
    /* Create options parser */
    Arc::OptionParser options;
    bool version(false);
    options.AddOption('v', "version", "print version information", version);
    options.AddOption('b', "bufsize", "size of buffer (per stream)", "buffer size", bufsize);
    options.AddOption('S', "rangestart", "range start", "start", range_start);
    options.AddOption('E', "rangeend", "range end", "end", range_end);
    options.AddOption('V', "verbosity", "logger verbosity level", "level", logger_verbosity);
    options.AddOption('F', "format", "logger output format", "format", logger_format);
    options.AddOption('s', "secure", "force secure data connection", "boolean", secure);
    options.AddOption('p', "passive", "force passive data connection", "boolean", passive);
    options.AddOption('o', "noorder", "allow out of order reading", "boolean", out_of_order);

    params = options.Parse(argc, argv);
    if (params.empty()) {
      if (version) {
        std::cout << Arc::IString("%s version %s", "arched", VERSION) << std::endl;
        exit(0);
      }
      std::cerr << Arc::IString("Expecting Command and URL provided");
      exit(1);
    }
  } catch(...) {
    std::cerr << "Arguments parsing error" << std::endl;
  }
  if(params.empty()) {
    std::cerr << Arc::IString("Expecting Command among arguments"); exit(1);
  }
  std::string command = params.front(); params.pop_front();
  if(params.empty()) {
    std::cerr << Arc::IString("Expecting URL among arguments"); exit(1);
  }
  if(logger_format >= 0) {
    logcerr.setFormat(static_cast<Arc::LogFormat>(logger_format));
  }
  logcerr.setPrefix("[external] ");
  if(!logger_verbosity.empty()) {
    Arc::Logger::getRootLogger().setThreshold(Arc::string_to_level(logger_verbosity));
    ArcDMCGridFTP::DataPointGridFTPHelper::logger.setThreshold(Arc::string_to_level(logger_verbosity));
  }

  std::string url_str = params.front(); params.pop_front();

  try {
    Arc::UserConfig usercfg;
    if(!DataExternalComm::InEntry(std::cin, usercfg)) {
      throw Arc::DataStatus(Arc::DataStatus::GenericError, "Failed to receive configuration");
    }

    ArcDMCGridFTP::DataPointGridFTPHelper* handler = new ArcDMCGridFTP::DataPointGridFTPHelper(url_str, usercfg, std::cin, std::cout);
    if(!*handler) {
      throw Arc::DataStatus(Arc::DataStatus::GenericError, "Failed to start protocol handler for URL "+url_str);
    }
    handler->SetBufferSize(bufsize);
    // handler->SetStreams(streams); - use URL instead
    handler->SetRange(range_start, range_end);
    handler->SetSecure(secure);
    handler->SetPassive(passive);
    handler->ReadOutOfOrder(out_of_order);
    Arc::DataStatus result(Arc::DataStatus::Success);
    if(command == Arc::DataPointDelegate::RenameCommand) {
      if(params.empty()) {
        throw Arc::DataStatus(Arc::DataStatus::GenericError, "Expecting new URL among arguments");
      }
      std::string new_url_str = params.front(); params.pop_front();
      if(!params.empty()) {
        throw Arc::DataStatus(Arc::DataStatus::GenericError, "Unexpected arguments");
      }
      result = handler->Rename(new_url_str);
    } else if(command == Arc::DataPointDelegate::ListCommand) {
      Arc::DataPoint::DataPointInfoType verb = Arc::DataPoint::INFO_TYPE_ALL;
      if(!params.empty()) {
        verb = static_cast<Arc::DataPoint::DataPointInfoType>(Arc::stringtoi(params.front()));
        params.pop_front();
      }
      if(!params.empty()) {
        throw Arc::DataStatus(Arc::DataStatus::GenericError, "Unexpected arguments");
      }
      result = handler->List(verb);
    } else if(command == Arc::DataPointDelegate::StatCommand) {
      Arc::DataPoint::DataPointInfoType verb = Arc::DataPoint::INFO_TYPE_ALL;
      if(!params.empty()) {
        verb = static_cast<Arc::DataPoint::DataPointInfoType>(Arc::stringtoi(params.front()));
        params.pop_front();
      }
      if(!params.empty()) {
        throw Arc::DataStatus(Arc::DataStatus::GenericError, "Unexpected arguments");
      }
      result = handler->Stat(verb);
    } else {
      if(!params.empty()) {
        throw Arc::DataStatus(Arc::DataStatus::GenericError, "Unexpected arguments");
      }
      if(command == Arc::DataPointDelegate::ReadCommand) {
        result = handler->Read();
      } else if(command == Arc::DataPointDelegate::WriteCommand) {
        result = handler->Write();
      } else if(command == Arc::DataPointDelegate::CheckCommand) {
        result = handler->Check();
      } else if(command == Arc::DataPointDelegate::RemoveCommand) {
        result = handler->Remove();
      } else if(command == Arc::DataPointDelegate::MkdirCommand) {
        result = handler->CreateDirectory(false);
      } else if(command == Arc::DataPointDelegate::MkdirRecursiveCommand) {
        result = handler->CreateDirectory(true);
      } else {
        throw Arc::DataStatus(Arc::DataStatus::GenericError, "Unknown command "+command);
      }
    }
    DataExternalComm::OutEntry(std::cout<<DataExternalComm::DataStatusTag, result);
    std::cerr.flush();
    std::cout.flush();
    _exit(0);
  } catch(Arc::DataStatus const& status) {
    DataExternalComm::OutEntry(std::cout<<DataExternalComm::DataStatusTag, status);
    std::cerr.flush();
    std::cout.flush();
    _exit(0);
  } catch(...) {
  }
  std::cerr.flush();
  std::cout.flush();
  _exit(-1);
}


