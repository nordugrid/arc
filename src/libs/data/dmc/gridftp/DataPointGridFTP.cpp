#include "common/Logger.h"
#include "common/StringConv.h"

#include "data/DataBufferPar.h"
#include "DataPointGridFTP.h"
#include "GlobusErrorUtils.h"
#include "Lister.h"

#define RESULT_SUCCESS 0
#define RESULT_ERROR 1

static inline Glib::TimeVal x(int s) {
  Glib::TimeVal t;
  t.assign_current_time();
  return t + s;
}

namespace Arc {

  Logger DataPointGridFTP::logger(DataPoint::logger, "GridFTP");

  void DataPointGridFTP::ftp_complete_callback
  (void *arg, globus_ftp_client_handle_t *handle, globus_object_t *error) {
    DataPointGridFTP *it = (DataPointGridFTP*)arg;
    if(error == GLOBUS_SUCCESS) {
      logger.msg(VERBOSE, "ftp_complete_callback: success");
      it->condstatus = RESULT_SUCCESS;
      it->cond.signal();
    }
    else {
      char *tmp = globus_object_printable_to_string(error);
      logger.msg(INFO, "ftp_complete_callback: error: %s", tmp); free(tmp);
      it->condstatus = RESULT_ERROR;
      it->cond.signal();
    }
  }

  void DataPointGridFTP::ftp_check_callback
  (void *arg, globus_ftp_client_handle_t *handle, globus_object_t *error,
   globus_byte_t *buffer, globus_size_t length, globus_off_t offset,
   globus_bool_t eof) {
    logger.msg(DEBUG, "ftp_check_callback");
    DataPointGridFTP *it = (DataPointGridFTP*)arg;
    if(error != GLOBUS_SUCCESS) {
      logger.msg(DEBUG, "Globus error: %s", error);
      return;
    }
    if(eof) return;
    globus_result_t res =
      globus_ftp_client_register_read(&(it->ftp_handle),
				      (globus_byte_t*)(it->ftp_buf),
				      sizeof(it->ftp_buf),
				      &ftp_check_callback, it);
    if(res != GLOBUS_SUCCESS) {
      logger.msg(INFO,
		 "Registration of Globus FTP buffer failed - cancel check");
      logger.msg(DEBUG, "Globus error: %s", GlobusResult(res).str().c_str());
      globus_ftp_client_abort(&(it->ftp_handle));
      return;
    }
    return;
  }

  /* check if user is allowed to get specified file. */
  bool DataPointGridFTP::check(void) {
    if(!DataPointDirect::check()) return false;
    globus_result_t res;
    globus_off_t size = 0;
    globus_abstime_t gl_modify_time;
    time_t modify_time;
    int modify_utime;
    bool size_available = false;
    res = globus_ftp_client_size(&ftp_handle, url.CanonicalURL().c_str(),
				 &ftp_opattr, &size,
				 &ftp_complete_callback, this);
    if(res != GLOBUS_SUCCESS) {
      logger.msg(DEBUG, "check_ftp: globus_ftp_client_size failed");
      logger.msg(INFO, "Globus error: %s", GlobusResult(res).str().c_str());
      // return false;
    }
    else if(!cond.timed_wait(mutex, x(300))) { /* 5 minutes timeout */
      logger.msg(INFO, "check_ftp: timeout waiting for size");
      globus_ftp_client_abort(&ftp_handle);
      cond.wait(mutex);
      // return false;
    }
    else if(condstatus != 0) {
      logger.msg(INFO, "check_ftp: failed to get file's size");
      // return false;
    }
    else {
      meta_size(size);
      size_available = true;
    }
    res = globus_ftp_client_modification_time(&ftp_handle,
					      url.CanonicalURL().c_str(),
					      &ftp_opattr, &gl_modify_time,
					      &ftp_complete_callback, this);
    if(res != GLOBUS_SUCCESS) {
      logger.msg(DEBUG,
		 "check_ftp: globus_ftp_client_modification_time failed");
      logger.msg(INFO, "Globus error: %s", GlobusResult(res).str().c_str());
      // return false;
    }
    else if(!cond.timed_wait(mutex, x(300))) { /* 5 minutes timeout */
      logger.msg(INFO, "check_ftp: timeout waiting for modification_time");
      globus_ftp_client_abort(&ftp_handle);
      cond.wait(mutex);
      // return false;
    }
    else if(condstatus != 0) {
      logger.msg(INFO, "check_ftp: failed to get file's modification time");
      // return false;
    }
    else {
      GlobusTimeAbstimeGet(gl_modify_time, modify_time, modify_utime);
      meta_created(modify_time);
    }
    // Do not use partial_get for ordinary ftp. Stupid globus tries to
    // use non-standard commands anyway.
    if(is_secure) {
      res = globus_ftp_client_partial_get(&ftp_handle,
					  url.CanonicalURL().c_str(),
					  &ftp_opattr, GLOBUS_NULL, 0, 1,
					  &ftp_complete_callback, this);
      if(res != GLOBUS_SUCCESS) {
	logger.msg(DEBUG, "check_ftp: globus_ftp_client_get failed");
	logger.msg(INFO, "Globus error: %s", GlobusResult(res).str().c_str());
	return false;
      }
      ftp_eof_flag = false;/* use eof_flag to pass result from callback */
      logger.msg(DEBUG, "check_ftp: globus_ftp_client_register_read");
      res = globus_ftp_client_register_read(&ftp_handle,
					    (globus_byte_t*)ftp_buf,
					    sizeof(ftp_buf),
					    &ftp_check_callback, this);
      if(res != GLOBUS_SUCCESS) {
	globus_ftp_client_abort(&ftp_handle);
	cond.wait(mutex);
	return false;
      }
      if(!cond.timed_wait(mutex, x(300))) {/* 5 minutes timeout */
	logger.msg(INFO, "check_ftp: timeout waiting for partial get");
	globus_ftp_client_abort(&ftp_handle);
	cond.wait(mutex);
	return false;
      }
      return (condstatus == 0);
    }
    else {
      // Do not use it at all. It does not give too much usefull
      // information anyway. But request at least existence of file.
      if(!size_available) return false;
      return true;
    }
  }

  bool DataPointGridFTP::remove(void) {
    if(!DataPointDirect::remove()) return false;
    //char buf[32];
    globus_result_t res;
    res = globus_ftp_client_delete(&ftp_handle, url.CanonicalURL().c_str(),
				   &ftp_opattr, &ftp_complete_callback, this);
    if(res != GLOBUS_SUCCESS) {
      logger.msg(DEBUG, "delete_ftp: globus_ftp_client_delete failed");
      logger.msg(INFO, "Globus error: %s", GlobusResult(res).str().c_str());
      return false;
    }
    if(!cond.timed_wait(mutex, x(300))) {/* 5 minutes timeout */
      logger.msg(INFO, "delete_ftp: globus_ftp_client_delete timeout");
      globus_ftp_client_abort(&ftp_handle);
      cond.wait(mutex);
      return false;
    }
    return (condstatus == 0);
  }

  static bool remove_last_dir(std::string& dir) {
    // dir also contains proto and server
    std::string::size_type nn = std::string::npos;
    if(!strncasecmp(dir.c_str(), "ftp://", 6)) {
      nn = dir.find('/', 6);
    }
    else if(!strncasecmp(dir.c_str(), "gsiftp://", 9)) {
      nn = dir.find('/', 9);
    }
    if(nn == std::string::npos) return false;
    std::string::size_type n;
    if((n = dir.rfind('/')) == std::string::npos) return false;
    if(n < nn) return false;
    dir.resize(n);
    return true;
  }

  static bool add_last_dir(std::string& dir, const std::string& path) {
    int l = dir.length();
    std::string::size_type n = path.find('/', l + 1);
    if(n == std::string::npos) return false;
    dir = path; dir.resize(n);
    return true;
  }

  bool DataPointGridFTP::mkdir_ftp() {
    ftp_dir_path = url.CanonicalURL();
    for(;;) if(!remove_last_dir(ftp_dir_path)) break;
    bool result = false;
    for(;;) {
      if(!add_last_dir(ftp_dir_path, url.CanonicalURL())) break;
      logger.msg(DEBUG, "mkdir_ftp: making &s", ftp_dir_path.c_str());
      globus_result_t res =
	globus_ftp_client_mkdir(&ftp_handle, ftp_dir_path.c_str(), &ftp_opattr,
				&ftp_complete_callback, this);
      if(res != GLOBUS_SUCCESS) {
	logger.msg(INFO, "Globus error: %s", GlobusResult(res).str().c_str());
	return false;
      }
      if(!cond.timed_wait(mutex, x(300))) {
	logger.msg(INFO, "mkdir_ftp: timeout waiting for mkdir");
	/* timeout - have to cancel operation here */
	globus_ftp_client_abort(&ftp_handle);
	cond.wait(mutex);
	return false;
      }
      // if(condstatus != 0) return false;
      result = result || (condstatus == 0);
    }
    return result;
    // return true;
  }

  bool DataPointGridFTP::start_reading(DataBufferPar& buf) {
    if(!DataPointDirect::start_reading(buf)) return false;
    buffer = &buf;
    /* size of file first */
    globus_off_t size = 0;
    bool limit_length = false;
    unsigned long long int range_length;
    if(range_end > range_start) {
      range_length = range_end - range_start;
      limit_length = true;
    }
    logger.msg(DEBUG, "start_reading_ftp");
    ftp_eof_flag = false;
    globus_ftp_client_handle_cache_url_state(&ftp_handle,
					     url.CanonicalURL().c_str());
    globus_result_t res;
    if((!no_checks) && (!(meta_size_available()))) {
      logger.msg(DEBUG, "start_reading_ftp: size: url: %s",
		 url.CanonicalURL().c_str());
      res = globus_ftp_client_size(&ftp_handle, url.CanonicalURL().c_str(),
              &ftp_opattr, &size, &ftp_complete_callback, this);
      if(res != GLOBUS_SUCCESS) {
	logger.msg(ERROR, "start_reading_ftp: failure");
	logger.msg(INFO, "Globus error: %s", GlobusResult(res).str().c_str());
	// globus_ftp_client_handle_flush_url_state(&ftp_handle,
	//					 url.CanonicalURL().c_str());
	// buffer->error_read(true);
	// DataPointDirect::stop_reading();
	// return false;
      }
      else if(!cond.timed_wait(mutex, x(300))) {
	logger.msg(ERROR, "start_reading_ftp: timeout waiting for file size");
	/* timeout - have to cancel operation here */
	logger.msg(INFO,
		   "Timeout waiting for FTP file size - cancel transfer");
	globus_ftp_client_abort(&ftp_handle);
	// have to do something in addition if complete callback will be called
	cond.wait(mutex);
	// globus_ftp_client_handle_flush_url_state(&ftp_handle,
	//					 url.CanonicalURL().c_str());
	// buffer->error_read(true);
	// DataPointDirect::stop_reading();
	// return false;
      }
      else if(condstatus != 0) {
	logger.msg(INFO, "start_reading_ftp: failed to get file's size");
	// buffer->error_read(true);
	// DataPointDirect::stop_reading();
	// return false;
      }
      else {
	/* provide some metadata */
	logger.msg(INFO, "start_reading_ftp: obtained size: %ull", size);
	meta_size(size);
      }
    }
    if((!no_checks) && (!(meta_created_available()))) {
      globus_abstime_t gl_modify_time;
      res = globus_ftp_client_modification_time(&ftp_handle,
						url.CanonicalURL().c_str(),
              &ftp_opattr, &gl_modify_time, &ftp_complete_callback, this);
      if(res != GLOBUS_SUCCESS) {
	logger.msg(DEBUG, "start_reading_ftp: "
		   "globus_ftp_client_modification_time failed");
	logger.msg(INFO, "Globus error: %s", GlobusResult(res).str().c_str());
	// buffer->error_read(true);
	// DataPointDirect::stop_reading();
	// return false;
      }
      else if(!cond.timed_wait(mutex, x(300))) { /* 5 minutes timeout */
	logger.msg(INFO, "start_reading_ftp: "
		   "timeout waiting for modification_time");
	globus_ftp_client_abort(&ftp_handle);
	cond.wait(mutex);
	// globus_ftp_client_handle_flush_url_state(&ftp_handle,
	//					 url.CanonicalURL().c_str());
	// buffer->error_read(true);
	// DataPointDirect::stop_reading();
	// return false;
      }
      if(condstatus != 0) {
	logger.msg(INFO, "start_reading_ftp: "
		   "failed to get file's modification time");
	// buffer->error_read(true);
	// DataPointDirect::stop_reading();
	// return false;
      }
      else {
	time_t modify_time;
	int modify_utime;
	GlobusTimeAbstimeGet(gl_modify_time, modify_time, modify_utime);
	logger.msg(DEBUG, "start_reading_ftp: creation time: %s", modify_time);
	meta_created(modify_time);
      }
      if(limit_length) {
	if(size < range_end) {
	  if(size <= range_start) { // report eof immediately
	    logger.msg(DEBUG, "start_reading_ftp: range is out of size");
	    buffer->eof_read(true);
	    condstatus = RESULT_SUCCESS;
	    cond.signal();
	    return true;
	  }
	  range_length = size - range_start;
	}
      }
    }
    logger.msg(DEBUG, "start_reading_ftp: globus_ftp_client_get");
    if(limit_length) {
      res = globus_ftp_client_partial_get
	(&ftp_handle, url.CanonicalURL().c_str(), &ftp_opattr,
	 GLOBUS_NULL, range_start, range_start + range_length + 1,
	 &ftp_get_complete_callback, this);
    }
    else {
      res = globus_ftp_client_get(&ftp_handle, url.CanonicalURL().c_str(),
				  &ftp_opattr, GLOBUS_NULL,
				  &ftp_get_complete_callback, this);
    }
    if(res != GLOBUS_SUCCESS) {
      logger.msg(DEBUG, "start_reading_ftp: globus_ftp_client_get failed");
      logger.msg(INFO, "Globus error: %s", GlobusResult(res).str().c_str());
      globus_ftp_client_handle_flush_url_state(&ftp_handle,
					       url.CanonicalURL().c_str());
      buffer->error_read(true);
      DataPointDirect::stop_reading();
      return false;
    }
    if(globus_thread_create(&ftp_control_thread, GLOBUS_NULL,
			    &ftp_read_thread, this) != 0) {
      logger.msg(DEBUG, "start_reading_ftp: globus_thread_create failed");
      globus_ftp_client_abort(&ftp_handle);
      cond.wait(mutex);
      globus_ftp_client_handle_flush_url_state(&ftp_handle,
					       url.CanonicalURL().c_str());
      buffer->error_read(true);
      DataPointDirect::stop_reading();
      return false;
    }
    /* make sure globus has thread for handling network/callbacks */
    globus_thread_blocking_will_block();
    return true;
  }

  bool DataPointGridFTP::stop_reading(void) {
    if(!DataPointDirect::stop_reading()) return false;
    if(!buffer->eof_read()) {
      logger.msg(DEBUG, "stop_reading_ftp: aborting connection");
      globus_ftp_client_abort(&ftp_handle);
    }
    logger.msg(DEBUG, "stop_reading_ftp: waiting for transfer to finish");
    cond.wait(mutex);
    logger.msg(DEBUG, "stop_reading_ftp: exiting: %s",
	       url.CanonicalURL().c_str());
    globus_ftp_client_handle_flush_url_state(&ftp_handle,
					     url.CanonicalURL().c_str());
    return true;
  }

  void *DataPointGridFTP::ftp_read_thread(void *arg) {
    DataPointGridFTP *it = (DataPointGridFTP*)arg;
    int h;
    unsigned int l;
    globus_result_t res;
    int registration_failed = 0;
    logger.msg(INFO, "ftp_read_thread: get and register buffers");
    int n_buffers = 0;
    for(;;) {
      if(it->buffer->eof_read()) break;
      if(!it->buffer->for_read(h, l, true)) {/* eof or error */
	if(it->buffer->error()) {/* error -> abort reading */
	  logger.msg(DEBUG, "ftp_read_thread: for_read failed - aborting: %s",
		     it->url.CanonicalURL().c_str());
	  globus_ftp_client_abort(&(it->ftp_handle));
	}
	break;
      }
      res = globus_ftp_client_register_read(&(it->ftp_handle),
              (globus_byte_t*)((*(it->buffer))[h]), l,
              &(it->ftp_read_callback), it);
      if(res != GLOBUS_SUCCESS) {
	logger.msg(VERBOSE, "ftp_read_thread: Globus error: %s",
		   GlobusResult(res).str().c_str());
	globus_object_t *err = globus_error_get(res);
	registration_failed++;
	if(registration_failed >= 10) {
	  it->buffer->is_read(h, 0, 0);
	  it->buffer->error_read(true);
	  // can set eof here because no callback will be called (I guess).
	  it->buffer->eof_read(true);
	  logger.msg(VERBOSE, "ftp_read_thread: "
		     "too many registration failures - abort: %s",
		     it->url.CanonicalURL().c_str());
	}
	else {
	  logger.msg(VERBOSE, "ftp_read_thread: "
		     "failed to register globus buffer - will try later: %s",
		     it->url.CanonicalURL().c_str());
	  it->buffer->is_read(h, 0, 0);
	  sleep(1);
	}
      }
      else {
	n_buffers++;
      }
    }
    /* make sure complete callback is called */
    logger.msg(DEBUG, "ftp_read_thread: waiting for eof");
    it->buffer->wait_eof_read();
    logger.msg(DEBUG, "ftp_read_thread: exiting");
    it->condstatus = it->buffer->error_read() ? 1 : 0;
    it->cond.signal();
    return NULL;
  }

  void DataPointGridFTP::ftp_read_callback
  (void *arg, globus_ftp_client_handle_t *handle, globus_object_t *error,
   globus_byte_t *buffer, globus_size_t length, globus_off_t offset,
   globus_bool_t eof) {
    DataPointGridFTP *it = (DataPointGridFTP*)arg;
    if(error != GLOBUS_SUCCESS) {
      logger.msg(DEBUG, "ftp_read_callback: failure");
      it->buffer->is_read((char*)buffer, 0, 0);
      return;
    }
    logger.msg(VERBOSE, "ftp_read_callback: success");
    it->buffer->is_read((char*)buffer, length, offset);
    if(eof) it->ftp_eof_flag = true;
    return;
  }

  void DataPointGridFTP::ftp_get_complete_callback
  (void *arg, globus_ftp_client_handle_t *handle, globus_object_t *error) {
    logger.msg(DEBUG, "ftp_get_complete_callback");
    DataPointGridFTP *it = (DataPointGridFTP*)arg;
    /* data transfer finished */
    if(error != GLOBUS_SUCCESS) {
      logger.msg(INFO, "Failed to get ftp file");
      globus_object_to_string(error, it->failure_description);
      logger.msg(DEBUG, "Globus error: %s", it->failure_description.c_str());
      it->buffer->error_read(true);
      return;
    }
    it->buffer->eof_read(true);
    return;
  }

  bool DataPointGridFTP::start_writing(DataBufferPar& buf,
				       DataCallback *space_cb) {
    if(!DataPointDirect::start_writing(buf)) return false;
    buffer = &buf;
    /* size of file first */
    //globus_off_t size;
    bool limit_length = false;
    unsigned long long int range_length;
    if(range_end > range_start) {
      range_length = range_end - range_start;
      limit_length = true;
    }
    logger.msg(DEBUG, "start_writing_ftp");
    ftp_eof_flag = false;
    globus_result_t res;
    /* !!!! TODO - preallocate here */

    globus_ftp_client_handle_cache_url_state(&ftp_handle,
					     url.CanonicalURL().c_str());
    if(!no_checks) {
      logger.msg(DEBUG, "start_writing_ftp: mkdir");
      if(!DataPointGridFTP::mkdir_ftp()) {
	logger.msg(DEBUG,
		   "start_writing_ftp: mkdir failed - still trying to write");
      }
    }
    logger.msg(DEBUG, "start_writing_ftp: put");
    if(limit_length) {
      res = globus_ftp_client_partial_put
	(&ftp_handle, url.CanonicalURL().c_str(), &ftp_opattr, GLOBUS_NULL,
	 range_start, range_start + range_length,
	 &ftp_put_complete_callback, this);
    }
    else {
      res = globus_ftp_client_put(&ftp_handle, url.CanonicalURL().c_str(),
				  &ftp_opattr, GLOBUS_NULL,
				  &ftp_put_complete_callback, this);
    }
    if(res != GLOBUS_SUCCESS) {
      logger.msg(DEBUG, "start_writing_ftp: put failed");
      failure_description = GlobusResult(res).str();
      logger.msg(INFO, "Globus error: %s", failure_description.c_str());
      globus_ftp_client_handle_flush_url_state(&ftp_handle,
					       url.CanonicalURL().c_str());
      buffer->error_write(true);
      DataPointDirect::stop_writing();
      return false;
    }
    if(globus_thread_create(&ftp_control_thread, GLOBUS_NULL,
			    &ftp_write_thread, this) != 0) {
      logger.msg(DEBUG, "start_writing_ftp: globus_thread_create failed");
      globus_ftp_client_handle_flush_url_state(&ftp_handle,
					       url.CanonicalURL().c_str());
      buffer->error_write(true);
      DataPointDirect::stop_writing();
      return false;
    }
    /* make sure globus has thread for handling network/callbacks */
    globus_thread_blocking_will_block();
    return true;
  }

  bool DataPointGridFTP::stop_writing(void) {
    if(!DataPointDirect::stop_writing()) return false;
    if(!buffer->eof_write()) {
      globus_ftp_client_abort(&ftp_handle);
    }
    cond.wait(mutex);
    globus_ftp_client_handle_flush_url_state(&ftp_handle,
					     url.CanonicalURL().c_str());
    return true;
  }

  void *DataPointGridFTP::ftp_write_thread(void *arg) {
    DataPointGridFTP *it = (DataPointGridFTP*)arg;
    int h;
    unsigned int l;
    unsigned long long int o;
    globus_result_t res;
    globus_bool_t eof = GLOBUS_FALSE;
    logger.msg(INFO, "ftp_write_thread: get and register buffers");
    //int n_buffers = 0;
    for(;;) {
      if(!it->buffer->for_write(h, l, o, true)) {
	if(it->buffer->error()) {
	  logger.msg(DEBUG, "ftp_write_thread: for_write failed - aborting");
	  globus_ftp_client_abort(&(it->ftp_handle));
	  break;
	}
	// no buffers and no errors - must be pure eof
	eof = GLOBUS_TRUE;
	char dummy;
	o = it->buffer->eof_position();
	res = globus_ftp_client_register_write(&(it->ftp_handle),
	        (globus_byte_t*)(&dummy), 0
	        , o, eof, &ftp_write_callback, it);
	break;
	// if(res == GLOBUS_SUCCESS) break;
	// sleep(1); continue;
      }
      res = globus_ftp_client_register_write(&(it->ftp_handle),
              (globus_byte_t*)((*(it->buffer))[h]), l
              , o, eof, &ftp_write_callback, it);
      if(res != GLOBUS_SUCCESS) {
	it->buffer->is_notwritten(h);
	sleep(1);
      }
    }
    /* make sure complete callback is called */
    it->buffer->wait_eof_write();
    it->condstatus = it->buffer->error_write() ? 1 : 0;
    it->cond.signal();
    return NULL;
  }

  void DataPointGridFTP::ftp_write_callback
  (void *arg, globus_ftp_client_handle_t *handle, globus_object_t *error,
   globus_byte_t *buffer, globus_size_t length, globus_off_t offset,
   globus_bool_t eof) {
    DataPointGridFTP *it = (DataPointGridFTP*)arg;
    // if(it->ftp_eof_flag) return; /* eof callback */
    if(error != GLOBUS_SUCCESS) {
      logger.msg(DEBUG, "ftp_write_callback: failure");
      it->buffer->is_written((char*)buffer);
      return;
    }
    logger.msg(VERBOSE, "ftp_write_callback: success");
    it->buffer->is_written((char*)buffer);
    return;
  }

  void DataPointGridFTP::ftp_put_complete_callback
  (void *arg, globus_ftp_client_handle_t *handle, globus_object_t *error) {
    logger.msg(DEBUG, "ftp_put_complete_callback");
    DataPointGridFTP *it = (DataPointGridFTP*)arg;
    /* data transfer finished */
    if(error != GLOBUS_SUCCESS) {
      logger.msg(INFO, "Failed to store ftp file");
      globus_object_to_string(error, it->failure_description);
      logger.msg(DEBUG, "Globus error: %s", it->failure_description.c_str());
      it->buffer->error_write(true);
      return;
    }
    it->buffer->eof_write(true);
    return;
  }

  bool DataPointGridFTP::list_files(std::list <DataPoint::FileInfo>& files,
				    bool resolve) {
    if(!DataPointDirect::list_files(files, resolve)) return false;
    Lister lister;
    if(lister.retrieve_dir(url.CanonicalURL()) != 0) {
      logger.msg(ERROR, "Failed to obtain listing from ftp: %s",
		 url.CanonicalURL().c_str());
      return false;
    }
    lister.close_connection();
    std::string base_url = url.CanonicalURL();
    std::string::size_type n = base_url.find("://");
    if(n == std::string::npos) {
      n = 0;
    }
    else {
      n += 3;
    }
    n = base_url.find('/', n);
    if(n != std::string::npos)
      base_url.resize(n);
    bool result = true;
    for(std::list <ListerFile>::iterator i = lister.begin();
	i != lister.end(); ++i) {
      std::list<DataPoint::FileInfo>::iterator f =
        files.insert(files.end(), DataPoint::FileInfo(i->GetLastName()));
      if(resolve) {
	globus_result_t res;
	globus_off_t size = 0;
	globus_abstime_t gl_modify_time;
	time_t modify_time;
	int modify_utime;
	// Lister should always return full path to file
	std::string f_url = base_url + i->GetName();
	f->type = (DataPoint::FileInfo::Type)(i->GetType());
	if(i->CheckSize()) {
	  f->size = i->GetSize();
	  f->size_available = true;
	}
	else if(i->GetType() != ListerFile::file_type_dir) {
	  logger.msg(VERBOSE, "list_files_ftp: looking for size of %s",
		     f_url.c_str());
	  res = globus_ftp_client_size(&ftp_handle, f_url.c_str(), &ftp_opattr,
	          &size, &ftp_complete_callback, this);
	  if(res != GLOBUS_SUCCESS) {
	    logger.msg(DEBUG, "list_files_ftp: globus_ftp_client_size failed");
	    logger.msg(INFO, "Globus error %s",
		       GlobusResult(res).str().c_str());
	    result = false;
	  }
	  else if(!cond.timed_wait(mutex, x(300))) { /* 5 minutes timeout */
	    logger.msg(INFO, "list_files_ftp: timeout waiting for size");
	    globus_ftp_client_abort(&ftp_handle);
	    cond.wait(mutex);
	    result = false;
	  }
	  else if(condstatus != 0) {
	    logger.msg(INFO, "list_files_ftp: failed to get file's size");
	    result = false;
	    // Guessing - directories usually have no size
	    f->type = DataPoint::FileInfo::file_type_dir;
	  }
	  else {
	    f->size = size;
	    f->size_available = true;
	    // Guessing - only files usually have size
	    f->type = DataPoint::FileInfo::file_type_file;
	  }
	}
	if(i->CheckCreated()) {
	  f->created = i->GetCreated();
	  f->created_available = true;
	}
	else {
	  logger.msg(VERBOSE, "list_files_ftp: "
		     "looking for modification time of %s", f_url.c_str());
	  res = globus_ftp_client_modification_time(&ftp_handle, f_url.c_str(),
	          &ftp_opattr, &gl_modify_time, &ftp_complete_callback, this);
	  if(res != GLOBUS_SUCCESS) {
	    logger.msg(DEBUG, "list_files_ftp: "
		       "globus_ftp_client_modification_time failed");
	    logger.msg(INFO, "Globus error: %s",
		       GlobusResult(res).str().c_str());
	    result = false;
	  }
	  else if(!cond.timed_wait(mutex, x(300))) { /* 5 minutes timeout */
	    logger.msg(INFO, "list_files_ftp: "
		       "timeout waiting for modification_time");
	    globus_ftp_client_abort(&ftp_handle);
	    cond.wait(mutex);
	    result = false;
	  }
	  else if(condstatus != 0) {
	    logger.msg(INFO, "list_files_ftp: "
		       "failed to get file's modification time");
	    result = false;
	  }
	  else {
	    GlobusTimeAbstimeGet(gl_modify_time, modify_time, modify_utime);
	    f->created = modify_time;
	    f->created_available = true;
	  }
	}
      }
    }
    return result;
  }

  DataPointGridFTP::DataPointGridFTP(const URL& url) : DataPointDirect(url),
						       ftp_active(false) {}

  DataPointGridFTP::~DataPointGridFTP(void) {
    stop_reading();
    stop_writing();
    deinit_handle();
  }

  bool DataPointGridFTP::init_handle(void) {
    if(!DataPointDirect::init_handle()) return false;
    is_secure = false;
    if(url.Protocol() == "gsiftp") is_secure = true;
    if(!ftp_active) {
      globus_result_t res;
      if((res = globus_ftp_client_handle_init(
            &ftp_handle, GLOBUS_NULL)) != GLOBUS_SUCCESS) {
	logger.msg(ERROR, "init_handle: globus_ftp_client_handle_init failed");
	logger.msg(ERROR, "Globus error: %s", GlobusResult(res).str().c_str());
	ftp_active = false;
	return false;
      }
      if((res = globus_ftp_client_operationattr_init(
            &ftp_opattr)) != GLOBUS_SUCCESS) {
	logger.msg(ERROR, "init_handle: "
		   "globus_ftp_client_operationattr_init failed");
	logger.msg(ERROR, "Globus error: %s",
		   GlobusResult(res).str().c_str());
	globus_ftp_client_handle_destroy(&ftp_handle);
	ftp_active = false;
	return false;
      }
    }
    ftp_active = true;
    ftp_threads = 1;
    if(allow_out_of_order) {
      ftp_threads = stringtoi(url.Option("threads"));
      if(ftp_threads < 1) ftp_threads = 1;
      if(ftp_threads > MAX_PARALLEL_STREAMS)
	ftp_threads = MAX_PARALLEL_STREAMS;
    }
    globus_ftp_control_parallelism_t paral;
    if(ftp_threads > 1) {
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
    if(!is_secure) { // plain ftp protocol
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

      char *subj = getenv("SUBJECT");
      if(subj) {
	globus_ftp_client_operationattr_set_authorization(&ftp_opattr,
	  GSS_C_NO_CREDENTIAL, NULL, NULL, NULL, subj);
      }
      if(force_secure || url.Option("secure") == "yes") {
	globus_ftp_client_operationattr_set_mode(&ftp_opattr,
	  GLOBUS_FTP_CONTROL_MODE_EXTENDED_BLOCK);
	globus_ftp_client_operationattr_set_data_protection(&ftp_opattr,
	  GLOBUS_FTP_CONTROL_PROTECTION_PRIVATE);
	logger.msg(DEBUG, "Using secure data transfer");
      }
      else {
	if(force_passive) {
	  globus_ftp_client_operationattr_set_mode(&ftp_opattr,
	    GLOBUS_FTP_CONTROL_MODE_STREAM);
	}
	else {
	  globus_ftp_client_operationattr_set_mode(&ftp_opattr,
	    GLOBUS_FTP_CONTROL_MODE_EXTENDED_BLOCK);
	}
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
    return true;
  }

  bool DataPointGridFTP::deinit_handle(void) {
    if(!DataPointDirect::deinit_handle()) return false;
    if(ftp_active) {
      logger.msg(DEBUG, "DataPoint::deinit_handle: destroy ftp_handle");
      globus_ftp_client_handle_destroy(&ftp_handle);
      globus_ftp_client_operationattr_destroy(&ftp_opattr);
    }
    return true;
  }

  bool DataPointGridFTP::analyze(analyze_t& arg) {
    return DataPointDirect::analyze(arg);
  }

  bool DataPointGridFTP::out_of_order(void) {
    return true;
  }

} // namespace Arc
