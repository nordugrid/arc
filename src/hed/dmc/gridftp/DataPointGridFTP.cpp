#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/Logger.h>
#include <arc/StringConv.h>
#include <arc/data/DataBuffer.h>
#include <arc/globusutils/GlobusErrorUtils.h>
#include <arc/globusutils/GSSCredential.h>

#include "DataPointGridFTP.h"
#include "Lister.h"

namespace Arc {

  Logger DataPointGridFTP::logger(DataPoint::logger, "GridFTP");

  void DataPointGridFTP::ftp_complete_callback(void *arg,
					       globus_ftp_client_handle_t*,
					       globus_object_t *error) {
    DataPointGridFTP *it = (DataPointGridFTP*)arg;
    if (error == GLOBUS_SUCCESS) {
      logger.msg(VERBOSE, "ftp_complete_callback: success");
      it->condstatus = DataStatus::Success;
      it->cond.signal();
    }
    else {
      logger.msg(DEBUG, "ftp_complete_callback: error: %s",
		 globus_object_to_string(error));
      it->condstatus = DataStatus::TransferError;
      it->cond.signal();
    }
  }

  void DataPointGridFTP::ftp_check_callback(void *arg,
					    globus_ftp_client_handle_t*,
					    globus_object_t *error,
					    globus_byte_t*,
					    globus_size_t,
					    globus_off_t,
					    globus_bool_t eof) {
    logger.msg(DEBUG, "ftp_check_callback");
    DataPointGridFTP *it = (DataPointGridFTP*)arg;
    if (error != GLOBUS_SUCCESS) {
      logger.msg(DEBUG, "Globus error: %s", globus_object_to_string(error));
      return;
    }
    if (eof)
      return;
    GlobusResult res =
      globus_ftp_client_register_read(&(it->ftp_handle),
				      (globus_byte_t*)(it->ftp_buf),
				      sizeof(it->ftp_buf),
				      &ftp_check_callback, it);
    if (!res) {
      logger.msg(INFO,
		 "Registration of Globus FTP buffer failed - cancel check");
      logger.msg(DEBUG, "Globus error: %s", res.str());
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
    int modify_utime;
    set_attributes();
    res = globus_ftp_client_size(&ftp_handle, url.str().c_str(), &ftp_opattr,
				 &size, &ftp_complete_callback, this);
    if (!res) {
      logger.msg(DEBUG, "check_ftp: globus_ftp_client_size failed");
      logger.msg(INFO, "Globus error: %s", res.str());
    }
    else if (!cond.wait(300000)) { /* 5 minutes timeout */
      logger.msg(INFO, "check_ftp: timeout waiting for size");
      globus_ftp_client_abort(&ftp_handle);
      cond.wait();
    }
    else if (!condstatus)
      logger.msg(INFO, "check_ftp: failed to get file's size");
    else
      SetSize(size);
    res = globus_ftp_client_modification_time(&ftp_handle, url.str().c_str(),
					      &ftp_opattr, &gl_modify_time,
					      &ftp_complete_callback, this);
    if (!res) {
      logger.msg(DEBUG,
		 "check_ftp: globus_ftp_client_modification_time failed");
      logger.msg(INFO, "Globus error: %s", res.str());
    }
    else if (!cond.wait(300000)) { /* 5 minutes timeout */
      logger.msg(INFO, "check_ftp: timeout waiting for modification_time");
      globus_ftp_client_abort(&ftp_handle);
      cond.wait();
    }
    else if (!condstatus)
      logger.msg(INFO, "check_ftp: failed to get file's modification time");
    else {
      GlobusTimeAbstimeGet(gl_modify_time, modify_time, modify_utime);
      SetCreated(modify_time);
    }
    // Do not use partial_get for ordinary ftp. Stupid globus tries to
    // use non-standard commands anyway.
    if (is_secure) {
      res = globus_ftp_client_partial_get(&ftp_handle, url.str().c_str(),
					  &ftp_opattr, GLOBUS_NULL, 0, 1,
					  &ftp_complete_callback, this);
      if (!res) {
	logger.msg(DEBUG, "check_ftp: globus_ftp_client_get failed");
	logger.msg(INFO, "Globus error: %s", res.str());
	return DataStatus::CheckError;
      }
      // use eof_flag to pass result from callback
      ftp_eof_flag = false;
      logger.msg(DEBUG, "check_ftp: globus_ftp_client_register_read");
      res = globus_ftp_client_register_read(&ftp_handle,
					    (globus_byte_t*)ftp_buf,
					    sizeof(ftp_buf),
					    &ftp_check_callback, this);
      if (!res) {
	globus_ftp_client_abort(&ftp_handle);
	cond.wait();
	return DataStatus::CheckError;
      }
      if (!cond.wait(300000)) { /* 5 minutes timeout */
	logger.msg(INFO, "check_ftp: timeout waiting for partial get");
	globus_ftp_client_abort(&ftp_handle);
	cond.wait();
	return DataStatus::CheckError;
      }
      return condstatus;
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
      logger.msg(DEBUG, "delete_ftp: globus_ftp_client_delete failed");
      logger.msg(INFO, "Globus error: %s", res.str());
      return DataStatus::DeleteError;
    }
    if (!cond.wait(300000)) { /* 5 minutes timeout */
      logger.msg(INFO, "delete_ftp: globus_ftp_client_delete timeout");
      globus_ftp_client_abort(&ftp_handle);
      cond.wait();
      return DataStatus::DeleteError;
    }
    return condstatus;
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
      logger.msg(DEBUG, "mkdir_ftp: making %s", ftp_dir_path);
      GlobusResult res =
	globus_ftp_client_mkdir(&ftp_handle, ftp_dir_path.c_str(), &ftp_opattr,
				&ftp_complete_callback, this);
      if (!res) {
	logger.msg(INFO, "Globus error: %s", res.str());
	return false;
      }
      if (!cond.wait(300000)) {
	logger.msg(INFO, "mkdir_ftp: timeout waiting for mkdir");
	/* timeout - have to cancel operation here */
	globus_ftp_client_abort(&ftp_handle);
	cond.wait();
	return false;
      }
      if (!condstatus)
	result = condstatus;
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
    /* size of file first */
    globus_off_t size = 0;
    bool limit_length = false;
    unsigned long long int range_length = 0;
    if (range_end > range_start) {
      range_length = range_end - range_start;
      limit_length = true;
    }
    logger.msg(DEBUG, "start_reading_ftp");
    ftp_eof_flag = false;
    globus_ftp_client_handle_cache_url_state(&ftp_handle, url.str().c_str());
    GlobusResult res;
    if ((additional_checks) && (!(CheckSize()))) {
      logger.msg(DEBUG, "start_reading_ftp: size: url: %s", url.str());
      res = globus_ftp_client_size(&ftp_handle, url.str().c_str(), &ftp_opattr,
				   &size, &ftp_complete_callback, this);
      if (!res) {
	logger.msg(ERROR, "start_reading_ftp: failure");
	logger.msg(INFO, "Globus error: %s", res.str());
	// globus_ftp_client_handle_flush_url_state(&ftp_handle,
	//                                          url.str().c_str());
	// buffer->error_read(true);
	// reading = false;
	// return DataStatus::ReadStartError;
      }
      else if (!cond.wait(300000)) {
	logger.msg(ERROR, "start_reading_ftp: timeout waiting for file size");
	/* timeout - have to cancel operation here */
	logger.msg(INFO,
		   "Timeout waiting for FTP file size - cancel transfer");
	globus_ftp_client_abort(&ftp_handle);
	// have to do something in addition if complete callback will be called
	cond.wait();
	// globus_ftp_client_handle_flush_url_state(&ftp_handle,
	//                                          url.str().c_str());
	// buffer->error_read(true);
	// reading = false;
	// return DataStatus::ReadStartError;
      }
      else if (!condstatus)
	logger.msg(INFO, "start_reading_ftp: failed to get file's size");
      // buffer->error_read(true);
      // reading = false;
      // return read_start_error;
      else {
	/* provide some metadata */
	logger.msg(INFO, "start_reading_ftp: obtained size: %llu", size);
	SetSize(size);
      }
    }
    if ((additional_checks) && (!(CheckCreated()))) {
      globus_abstime_t gl_modify_time;
      res = globus_ftp_client_modification_time(&ftp_handle, url.str().c_str(),
						&ftp_opattr, &gl_modify_time,
						&ftp_complete_callback, this);
      if (!res) {
	logger.msg(DEBUG, "start_reading_ftp: "
		   "globus_ftp_client_modification_time failed");
	logger.msg(INFO, "Globus error: %s", res.str());
	// buffer->error_read(true);
	// reading = false;
	// return DataStatus::ReadStartError;
      }
      else if (!cond.wait(300000)) { /* 5 minutes timeout */
	logger.msg(INFO, "start_reading_ftp: "
		   "timeout waiting for modification_time");
	globus_ftp_client_abort(&ftp_handle);
	cond.wait();
	// globus_ftp_client_handle_flush_url_state(&ftp_handle,
	//                                          url.str().c_str());
	// buffer->error_read(true);
	// reading = false;
	// return DataStatus::ReadStartError;
      }
      if (!condstatus)
	logger.msg(INFO, "start_reading_ftp: "
		   "failed to get file's modification time");
      // buffer->error_read(true);
      // reading = false;
      // return condstatus;
      else {
	time_t modify_time;
	int modify_utime;
	GlobusTimeAbstimeGet(gl_modify_time, modify_time, modify_utime);
	SetCreated(modify_time);
	logger.msg(DEBUG, "start_reading_ftp: creation time: %s",
		   GetCreated().str());
      }
      if (limit_length)
	if ((unsigned long long int)size < range_end) {
	  if ((unsigned long long int)size <= range_start) {
	    // report eof immediately
	    logger.msg(DEBUG, "start_reading_ftp: range is out of size");
	    buffer->eof_read(true);
	    condstatus = DataStatus::Success;
	    cond.signal();
	    return DataStatus::Success;
	  }
	  range_length = size - range_start;
	}
    }
    logger.msg(DEBUG, "start_reading_ftp: globus_ftp_client_get");
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
      logger.msg(DEBUG, "start_reading_ftp: globus_ftp_client_get failed");
      logger.msg(INFO, "Globus error: %s", res.str());
      globus_ftp_client_handle_flush_url_state(&ftp_handle, url.str().c_str());
      buffer->error_read(true);
      reading = false;
      return DataStatus::ReadStartError;
    }
    if (globus_thread_create(&ftp_control_thread, GLOBUS_NULL,
			     &ftp_read_thread, this) != 0) {
      logger.msg(DEBUG, "start_reading_ftp: globus_thread_create failed");
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
      logger.msg(DEBUG, "stop_reading_ftp: aborting connection");
      globus_ftp_client_abort(&ftp_handle);
    }
    logger.msg(DEBUG, "stop_reading_ftp: waiting for transfer to finish");
    cond.wait();
    logger.msg(DEBUG, "stop_reading_ftp: exiting: %s", url.str());
    globus_ftp_client_handle_flush_url_state(&ftp_handle, url.str().c_str());
    return condstatus;
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
	  logger.msg(DEBUG, "ftp_read_thread: for_read failed - aborting: %s",
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
	logger.msg(VERBOSE, "ftp_read_thread: Globus error: %s", res.str());
	registration_failed++;
	if (registration_failed >= 10) {
	  it->buffer->is_read(h, 0, 0);
	  it->buffer->error_read(true);
	  // can set eof here because no callback will be called (I guess).
	  it->buffer->eof_read(true);
	  logger.msg(VERBOSE, "ftp_read_thread: "
		     "too many registration failures - abort: %s",
		     it->url.str());
	}
	else {
	  logger.msg(VERBOSE, "ftp_read_thread: "
		     "failed to register globus buffer - will try later: %s",
		     it->url.str());
	  it->buffer->is_read(h, 0, 0);
	  sleep(1);
	}
      }
      else
	n_buffers++;
    }
    /* make sure complete callback is called */
    logger.msg(DEBUG, "ftp_read_thread: waiting for eof");
    it->buffer->wait_eof_read();
    logger.msg(DEBUG, "ftp_read_thread: exiting");
    it->condstatus = it->buffer->error_read() ? DataStatus::ReadError :
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
    DataPointGridFTP *it = (DataPointGridFTP*)arg;
    if (error != GLOBUS_SUCCESS) {
      logger.msg(DEBUG, "ftp_read_callback: failure");
      it->buffer->is_read((char*)buffer, 0, 0);
      return;
    }
    logger.msg(VERBOSE, "ftp_read_callback: success");
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
      logger.msg(DEBUG, "Globus error: %s", globus_object_to_string(error));
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
    if (additional_checks) {
      logger.msg(DEBUG, "start_writing_ftp: mkdir");
      if (!mkdir_ftp())
	logger.msg(DEBUG,
		   "start_writing_ftp: mkdir failed - still trying to write");
    }
    logger.msg(DEBUG, "start_writing_ftp: put");
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
      logger.msg(DEBUG, "start_writing_ftp: put failed");
      logger.msg(INFO, "Globus error: %s", res.str());
      globus_ftp_client_handle_flush_url_state(&ftp_handle, url.str().c_str());
      buffer->error_write(true);
      writing = false;
      return DataStatus::WriteStartError;
    }
    if (globus_thread_create(&ftp_control_thread, GLOBUS_NULL,
			     &ftp_write_thread, this) != 0) {
      logger.msg(DEBUG, "start_writing_ftp: globus_thread_create failed");
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
      logger.msg(DEBUG, "StopWriting: aborting connection");
      globus_ftp_client_abort(&ftp_handle);
    }
    cond.wait();
    globus_ftp_client_handle_flush_url_state(&ftp_handle, url.str().c_str());
    return condstatus;
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
	  logger.msg(DEBUG, "ftp_write_thread: for_write failed - aborting");
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
    /* make sure complete callback is called */
    it->buffer->wait_eof_write();
    it->condstatus = it->buffer->error_write() ? DataStatus::WriteError :
		     DataStatus::Success;
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
      logger.msg(DEBUG, "ftp_write_callback: failure");
      it->buffer->is_written((char*)buffer);
      return;
    }
    logger.msg(VERBOSE, "ftp_write_callback: success");
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
      logger.msg(DEBUG, "Globus error: %s", globus_object_to_string(error));
      it->buffer->error_write(true);
      return;
    }
    it->buffer->eof_write(true);
    return;
  }

  DataStatus DataPointGridFTP::ListFiles(std::list<FileInfo>& files,
                                         bool long_list,
                                         bool resolve) {
    if (!ftp_active)
      return DataStatus::NotInitializedError;
    if (reading)
      return DataStatus::IsReadingError;
    if (writing)
      return DataStatus::IsWritingError;
    set_attributes();
    Lister lister(*credential);
    if (lister.retrieve_dir(url) != 0) {
      logger.msg(ERROR, "Failed to obtain listing from ftp: %s", url.str());
      return DataStatus::ListError;
    }
    lister.close_connection();
    DataStatus result = DataStatus::Success;
    for (std::list<FileInfo>::iterator i = lister.begin();
	 i != lister.end(); ++i) {
      std::list<FileInfo>::iterator f =
	files.insert(files.end(), FileInfo(i->GetLastName()));
      if (long_list) {
	GlobusResult res;
	globus_off_t size = 0;
	globus_abstime_t gl_modify_time;
	time_t modify_time;
	int modify_utime;
	// Lister should always return full path to file
	std::string f_url = url.ConnectionURL() + i->GetName();
	f->SetType(i->GetType());
	if (i->CheckSize())
	  f->SetSize(i->GetSize());
	else if (i->GetType() != FileInfo::file_type_dir) {
	  logger.msg(VERBOSE, "list_files_ftp: looking for size of %s", f_url);
	  res = globus_ftp_client_size(&ftp_handle, f_url.c_str(), &ftp_opattr,
				       &size, &ftp_complete_callback, this);
	  if (!res) {
	    logger.msg(DEBUG, "list_files_ftp: globus_ftp_client_size failed");
	    logger.msg(INFO, "Globus error: %s", res.str());
	    result = DataStatus::ListError;
	  }
	  else if (!cond.wait(300000)) { /* 5 minutes timeout */
	    logger.msg(INFO, "list_files_ftp: timeout waiting for size");
	    globus_ftp_client_abort(&ftp_handle);
	    cond.wait();
	    result = DataStatus::ListError;
	  }
	  else if (!condstatus) {
	    logger.msg(INFO, "list_files_ftp: failed to get file's size");
	    result = DataStatus::ListError;
	    // Guessing - directories usually have no size
	    f->SetType(FileInfo::file_type_dir);
	  }
	  else {
	    f->SetSize(size);
	    // Guessing - only files usually have size
	    f->SetType(FileInfo::file_type_file);
	  }
	}
	if (i->CheckCreated())
	  f->SetCreated(i->GetCreated());
	else {
	  logger.msg(VERBOSE, "list_files_ftp: "
		     "looking for modification time of %s", f_url);
	  res =
	    globus_ftp_client_modification_time(&ftp_handle, f_url.c_str(),
						&ftp_opattr, &gl_modify_time,
						&ftp_complete_callback, this);
	  if (!res) {
	    logger.msg(DEBUG, "list_files_ftp: "
		       "globus_ftp_client_modification_time failed");
	    logger.msg(INFO, "Globus error: %s", res.str());
	    result = DataStatus::ListError;
	  }
	  else if (!cond.wait(300000)) { /* 5 minutes timeout */
	    logger.msg(INFO, "list_files_ftp: "
		       "timeout waiting for modification_time");
	    globus_ftp_client_abort(&ftp_handle);
	    cond.wait();
	    result = DataStatus::ListError;
	  }
	  else if (!condstatus) {
	    logger.msg(INFO, "list_files_ftp: "
		       "failed to get file's modification time");
	    result = DataStatus::ListError;
	  }
	  else {
	    GlobusTimeAbstimeGet(gl_modify_time, modify_time, modify_utime);
	    f->SetCreated(modify_time);
	  }
	}
      }
    }
    return result;
  }

  DataPointGridFTP::DataPointGridFTP(const URL& url)
    : DataPointDirect(url),
      ftp_active(false),
      condstatus(DataStatus::Success),
      credential(NULL),
      reading(false),
      writing(false) {
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
	credential = new GSSCredential(proxyPath, certificatePath, keyPath);

      globus_ftp_client_operationattr_set_authorization(&ftp_opattr,
							*credential,
							":globus-mapping:",
							"user@",
							GLOBUS_NULL,
							GLOBUS_NULL);
      if (force_secure || (url.Option("secure") == "yes")) {
	globus_ftp_client_operationattr_set_mode(&ftp_opattr,
						 GLOBUS_FTP_CONTROL_MODE_EXTENDED_BLOCK);
	globus_ftp_client_operationattr_set_data_protection(&ftp_opattr,
							    GLOBUS_FTP_CONTROL_PROTECTION_PRIVATE);
	logger.msg(DEBUG, "Using secure data transfer");
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
	logger.msg(DEBUG, "Using insecure data transfer");
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
      globus_ftp_client_handle_destroy(&ftp_handle);
      globus_ftp_client_operationattr_destroy(&ftp_opattr);
    }
    if (credential)
      delete credential;
  }

  bool DataPointGridFTP::WriteOutOfOrder() {
    return true;
  }

} // namespace Arc
