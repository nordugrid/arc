#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <list>
#include <string>

#include <globus_common.h>
#include <globus_ftp_control.h>
#include <globus_object.h>

#include <arc/DateTime.h>
#include <arc/Logger.h>
#include <arc/StringConv.h>
#include <arc/URL.h>
#include <arc/data/FileInfo.h>
#include <arc/globusutils/GlobusErrorUtils.h>
#include <arc/globusutils/GSSCredential.h>

#include "Lister.h"


static char default_ftp_user[] = "ftp";
static char default_gsiftp_user[] = ":globus-mapping:";
static char default_ftp_pass[] = "user@";
static char default_gsiftp_pass[] = "user@";

static void dos_to_unix(char *s) {
  if (!s)
    return;
  int l = strlen(s);
  for (; l;) {
    l--;
    if ((s[l] == '\r') || (s[l] == '\n'))
      s[l] = 0;
  }
}

namespace Arc {

  static Logger logger(Logger::rootLogger, "Lister");

  bool SetAttributes(FileInfo& fi, const char *facts) {
    const char *name;
    const char *value;
    const char *p = facts;

    for (; *p;) {
      name = p;
      value = p;
      if (*p == ' ')
	break; // end of facts
      if (*p == ';') {
	p++;
	continue;
      }
      for (; *p; p++) {
	if (*p == ' ')
	  break;
	if (*p == ';')
	  break;
	if (*p == '=')
	  value = p;
      }
      if (name == value)
	continue; // skip empty names
      value++;
      if (value == p)
	continue; // skip empty values
      if (((value - name - 1) == 4) && (strncasecmp(name, "type", 4) == 0)) {
	if (((p - value) == 3) && (strncasecmp(value, "dir", 3) == 0))
	  fi.SetType(FileInfo::file_type_dir);
	else if (((p - value) == 4) && (strncasecmp(value, "file", 4) == 0))
	  fi.SetType(FileInfo::file_type_file);
	else
	  fi.SetType(FileInfo::file_type_unknown);
      }
      else if (((value - name - 1) == 4) &&
	       (strncasecmp(name, "size", 4) == 0)) {
	std::string tmp_s(value, (int)(p - value));
	fi.SetSize(stringtoull(tmp_s));
      }
      else if (((value - name - 1) == 6) &&
	       (strncasecmp(name, "modify", 6) == 0)) {
	std::string tmp_s(value, (int)(p - value));
	if (tmp_s.size() < 14)
	 fi.SetCreated(stringtoi(tmp_s)); // UNIX time
	 else
	  fi.SetCreated(tmp_s); // ISO time
      }
    }
    return true;
  }

  Lister::callback_status_t Lister::wait_for_callback() {
    callback_status_t res;
    globus_mutex_lock(&mutex);
    while (callback_status == CALLBACK_NOTREADY)
      globus_cond_wait(&cond, &mutex);
    res = callback_status;
    callback_status = CALLBACK_NOTREADY;
    globus_mutex_unlock(&mutex);
    return res;
  }

  Lister::callback_status_t Lister::wait_for_data_callback() {
    callback_status_t res;
    globus_mutex_lock(&mutex);
    while (data_callback_status == CALLBACK_NOTREADY)
      globus_cond_wait(&cond, &mutex);
    res = data_callback_status;
    data_callback_status = CALLBACK_NOTREADY;
    globus_mutex_unlock(&mutex);
    return res;
  }

  void Lister::resp_destroy() {
    globus_mutex_lock(&mutex);
    if (resp_n > 0) {
      globus_ftp_control_response_destroy(resp + (resp_n - 1));
      resp_n--;
    }
    globus_mutex_unlock(&mutex);
  }

  void Lister::resp_callback(void *arg, globus_ftp_control_handle_t*,
			     globus_object_t *error,
			     globus_ftp_control_response_t *response) {
    Lister *it = (Lister*)arg;
    globus_mutex_lock(&(it->mutex));
    if (error != GLOBUS_SUCCESS) {
      it->callback_status = CALLBACK_ERROR;
      std::string tmp = globus_object_to_string(error);
      logger.msg(INFO, "Failure: %s", tmp);
      if (response)
	logger.msg(INFO, "Response: %s", response->response_buffer);
    }
    else {
      if (it->resp_n < LISTER_MAX_RESPONSES) {
	memmove((it->resp) + 1, it->resp,
		sizeof(globus_ftp_control_response_t) * (it->resp_n));
	if (response->response_buffer)
	  globus_ftp_control_response_copy(response, it->resp);
	else {    // invalid reply causes *_copy to segfault
	  it->resp->response_buffer = (globus_byte_t*)strdup("000 ");
	  it->resp->response_buffer_size = 5;
	  it->resp->response_length = 4;
	  it->resp->code = 0;
	  it->resp->response_class = GLOBUS_FTP_UNKNOWN_REPLY;
	}
	(it->resp_n)++;
      }
      it->callback_status = CALLBACK_DONE;
      dos_to_unix((char*)(it->resp->response_buffer));
      logger.msg(DEBUG, "Response: %s", it->resp->response_buffer);
    }
    globus_cond_signal(&(it->cond));
    globus_mutex_unlock(&(it->mutex));
  }

  void Lister::list_read_callback(void *arg,
				  globus_ftp_control_handle_t*,
				  globus_object_t *error,
				  globus_byte_t*,
				  globus_size_t length,
				  globus_off_t,
				  globus_bool_t eof) {
    Lister *it = (Lister*)arg;
    length += it->list_shift;
    if (error != GLOBUS_SUCCESS) {
      /* no such file or connection error - assume no such file */
      logger.msg(INFO, "Error getting list of files (in list)");
      std::string tmp = globus_object_to_string(error);
      logger.msg(INFO, "Failure: %s", tmp);
      logger.msg(INFO, "Assuming - file not found");
      globus_mutex_lock(&(it->mutex));
      it->data_callback_status = CALLBACK_ERROR;
      globus_cond_signal(&(it->cond));
      globus_mutex_unlock(&(it->mutex));
      return;
    }
    /* parse names and add to list */
    /* suppose we are receiving ordered blocks of data (no multiple streams) */
    char *name;
    (it->readbuf)[length] = 0;
    name = it->readbuf;
    it->list_shift = 0;
    for (;;) {
      if ((*name) == 0)
	break;
      globus_size_t nlen;
      nlen = strcspn(name, "\n\r");
      name[nlen] = 0;
      logger.msg(DEBUG, "list record: %s", name);
      if (nlen == length)
	if (!eof) {
	  memmove(it->readbuf, name, nlen);
	  it->list_shift = nlen;
	  break;
	}
      if (nlen == 0) { // skip empty std::string
	if (length == 0)
	  break;
	name++;
	length--;
	continue;
      }
      char *attrs = name;
      if (it->facts)
	for (; *name;) {
	  nlen--;
	  length--;
	  if (*name == ' ') {
	    name++;
	    break;
	  }
	  name++;
	}
      std::list<FileInfo>::iterator i;
      if (name[0] == '/')
	i = it->fnames.insert(it->fnames.end(), FileInfo(name));
      else {
	std::string name_ = !it->path.empty() ? it->path : "/";
	name_ += "/";
	name_ += name;
	i = it->fnames.insert(it->fnames.end(), FileInfo(name_));
      }
      if (it->facts)
	SetAttributes(*i, attrs);
      if (nlen == length)
	break;
      name += (nlen + 1);
      length -= (nlen + 1);
      if (((*name) == '\r') || ((*name) == '\n')) {
	name++;
	length--;
      }
    }
    if (!eof) {
      if (globus_ftp_control_data_read(it->handle, (globus_byte_t*)
				       ((it->readbuf) + (it->list_shift)),
				       sizeof(it->readbuf) -
				       (it->list_shift) - 1,
				       &list_read_callback, arg) !=
	  GLOBUS_SUCCESS) {
	logger.msg(INFO, "Failed reading list of files");
	globus_mutex_lock(&(it->mutex));
	it->data_callback_status = CALLBACK_ERROR;
	globus_cond_signal(&(it->cond));
	globus_mutex_unlock(&(it->mutex));
      }
      return;
    }
    globus_mutex_lock(&(it->mutex));
    it->data_callback_status = CALLBACK_DONE;
    globus_cond_signal(&(it->cond));
    globus_mutex_unlock(&(it->mutex));
    return;
  }

  void Lister::list_conn_callback(void *arg,
				  globus_ftp_control_handle_t *hctrl,
				  unsigned int,
				  globus_bool_t,
				  globus_object_t *error) {
    /* if(!callback_active) return; */
    Lister *it = (Lister*)arg;
    if (error != GLOBUS_SUCCESS) {
      std::string tmp = globus_object_to_string(error);
      logger.msg(INFO, "Failure: %s", tmp);
      globus_mutex_lock(&(it->mutex));
      it->data_callback_status = CALLBACK_ERROR;
      globus_cond_signal(&(it->cond));
      globus_mutex_unlock(&(it->mutex));
      return;
    }
    it->list_shift = 0;
    it->fnames.clear();
    if (globus_ftp_control_data_read(hctrl, (globus_byte_t*)(it->readbuf),
				     sizeof(it->readbuf) - 1,
				     &list_read_callback, arg) !=
	GLOBUS_SUCCESS) {
      logger.msg(INFO, "Failed reading data");
      globus_mutex_lock(&(it->mutex));
      it->data_callback_status = CALLBACK_ERROR;
      globus_cond_signal(&(it->cond));
      globus_mutex_unlock(&(it->mutex));
      return;
    }
  }

  globus_ftp_control_response_class_t Lister::send_command(const char *command, const char *arg,
							   bool wait_for_response, char **sresp, char delim) {
    char *cmd = NULL;
    if (sresp)
      (*sresp) = NULL;
    if (command) { /* if no command - waiting for second reply */
      globus_mutex_lock(&mutex);
      for (int i = 0; i < resp_n; i++)
	globus_ftp_control_response_destroy(resp + i);
      resp_n = 0;
      callback_status = CALLBACK_NOTREADY;
      globus_mutex_unlock(&mutex);
      if (arg)
	cmd = (char*)malloc(strlen(arg) + strlen(command) + 4);
      else
	cmd = (char*)malloc(strlen(command) + 3);
      if (cmd == NULL) {
	logger.msg(ERROR, "Memory allocation error");
	return GLOBUS_FTP_UNKNOWN_REPLY;
      }
      strcpy(cmd, command);
      if (arg) {
	strcat(cmd, " ");
	strcat(cmd, arg);
      }
      logger.msg(DEBUG, "Command: %s", cmd);
      strcat(cmd, "\r\n");
      if (globus_ftp_control_send_command(handle, cmd, resp_callback, this)
	  != GLOBUS_SUCCESS) {
	logger.msg(DEBUG, "%s failed", command);
	if (cmd)
	  free(cmd);
	return GLOBUS_FTP_UNKNOWN_REPLY;
      }
      logger.msg(VERBOSE, "Command is being sent");
    }
    if (wait_for_response) {
      globus_mutex_lock(&mutex);
      while ((callback_status == CALLBACK_NOTREADY) && (resp_n == 0)) {
	logger.msg(VERBOSE, "Waiting for response");
	globus_cond_wait(&cond, &mutex);
      }
      free(cmd);
      if (callback_status != CALLBACK_DONE) {
	logger.msg(VERBOSE, "Callback got failure");
	callback_status = CALLBACK_NOTREADY;
	if (resp_n > 0) {
	  globus_ftp_control_response_destroy(resp + (resp_n - 1));
	  resp_n--;
	}
	globus_mutex_unlock(&mutex);
	return GLOBUS_FTP_UNKNOWN_REPLY;
      }
      if ((sresp) && (resp_n > 0)) {
	if (delim == 0) {
	  (*sresp) = (char*)malloc(resp[resp_n - 1].response_length);
	  if ((*sresp) != NULL) {
	    memcpy(*sresp, (char*)(resp[resp_n - 1].response_buffer + 4),
		   resp[resp_n - 1].response_length - 4);
	    (*sresp)[resp[resp_n - 1].response_length - 4] = 0;
	    logger.msg(DEBUG, "Response: %s", *sresp);
	  }
	  else
	    logger.msg(ERROR, "Memory allocation error");
	}
	else {
	  /* look for pair of enclosing characters */
	  logger.msg(DEBUG, "Response: %s", resp[resp_n - 1].response_buffer);
	  char *s_start = (char*)(resp[resp_n - 1].response_buffer + 4);
	  char *s_end = NULL;
	  int l = 0;
	  s_start = strchr(s_start, delim);
	  if (s_start) {
	    s_start++;
	    if (delim == '(')
	      delim = ')';
	    else if (delim == '{')
	      delim = '}';
	    else if (delim == '[')
	      delim = ']';
	    s_end = strchr(s_start, delim);
	    if (s_end)
	      l = s_end - s_start;
	  }
	  if (l > 0) {
	    (*sresp) = (char*)malloc(l + 1);
	    if ((*sresp) != NULL) {
	      memcpy(*sresp, s_start, l);
	      (*sresp)[l] = 0;
	      logger.msg(DEBUG, "Response: %s", *sresp);
	    }
	  }
	}
      }
      globus_ftp_control_response_class_t resp_class =
	GLOBUS_FTP_UNKNOWN_REPLY;
      if (resp_n > 0) {
	resp_class = resp[resp_n - 1].response_class;
	globus_ftp_control_response_destroy(resp + (resp_n - 1));
	resp_n--;
      }
      if (resp_n == 0)
	callback_status = CALLBACK_NOTREADY;
      globus_mutex_unlock(&mutex);
      return resp_class;
    }
    else
      return GLOBUS_FTP_POSITIVE_COMPLETION_REPLY;
    /* !!!!!!! Memory LOST - cmd !!!!!!!! */
  }

  Lister::Lister(GSSCredential& credential)
    : inited(false),
      handle(NULL),
      resp_n(0),
      callback_status(CALLBACK_NOTREADY),
      connected(false),
      credential(credential),
      port((unsigned short int)(-1)) {
    if (globus_cond_init(&cond, GLOBUS_NULL) != GLOBUS_SUCCESS) {
      logger.msg(ERROR, "Failed initing condition");
      return;
    }
    if (globus_mutex_init(&mutex, GLOBUS_NULL) != GLOBUS_SUCCESS) {
      logger.msg(ERROR, "Failed initing mutex");
      globus_cond_destroy(&cond);
      return;
    }
    handle = (globus_ftp_control_handle_t*)
	     malloc(sizeof(globus_ftp_control_handle_t));
    if (handle == NULL) {
      logger.msg(ERROR, "Failed allocating memory for handle");
      globus_mutex_destroy(&mutex);
      globus_cond_destroy(&cond);
    }
    if (globus_ftp_control_handle_init(handle) != GLOBUS_SUCCESS) {
      logger.msg(ERROR, "Failed initing handle");
      globus_mutex_destroy(&mutex);
      globus_cond_destroy(&cond);
      free(handle);
      handle = NULL;
      return;
    }
    inited = true;
  }

  int Lister::close_connection() {
    if (!connected)
      return 0;
    logger.msg(DEBUG, "Closing connection");
    if (globus_ftp_control_quit(handle, resp_callback, this) !=
	GLOBUS_SUCCESS)
      if (globus_ftp_control_force_close(handle, resp_callback, this) !=
	  GLOBUS_SUCCESS) {
	logger.msg(INFO, "Failed to close connection 1");
	return -1;
      }
    if (wait_for_callback() != CALLBACK_DONE) {
      if (globus_ftp_control_force_close(handle, resp_callback, this) !=
	  GLOBUS_SUCCESS) {
	logger.msg(INFO, "Failed to close connection 2");
	return -1;
      }
      if (wait_for_callback() != CALLBACK_DONE) {
	logger.msg(INFO, "Failed to close connection 3");
	return -1;
      }
    }
    connected = false;
    logger.msg(DEBUG, "Closed successfully");
    return 0;
  }

  Lister::~Lister() {
    close_connection();
    if (inited) {
      if (globus_ftp_control_handle_destroy(handle) == GLOBUS_SUCCESS) {
	free(handle);
	handle = NULL;
      }
      else {
	logger.msg(DEBUG, "Memory leak (globus_ftp_control_handle_t)");
	handle = NULL;
      }
      globus_mutex_destroy(&mutex);
      globus_cond_destroy(&cond);
    }
  }

  int Lister::setup_pasv(globus_ftp_control_host_port_t& pasv_addr) {
    char *sresp;
    GlobusResult res;
    if (send_command("PASV", NULL, true, &sresp, '(') !=
	GLOBUS_FTP_POSITIVE_COMPLETION_REPLY) {
      if (sresp) {
	logger.msg(INFO, "PASV failed: %s", sresp);
	free(sresp);
      }
      else
	logger.msg(INFO, "PASV failed");
      return -1;
    }
    pasv_addr.port = 0;
    if (sresp) {
      int port_low, port_high;
      if (sscanf(sresp, "%i,%i,%i,%i,%i,%i",
		 &(pasv_addr.host[0]), &(pasv_addr.host[1]),
		 &(pasv_addr.host[2]), &(pasv_addr.host[3]),
		 &port_high, &port_low) == 6)
	pasv_addr.port = ((port_high & 0x000FF) << 8) | (port_low & 0x000FF);
    }
    if (pasv_addr.port == 0) {
      logger.msg(INFO, "Can't parse host and port in response to PASV");
      if (sresp)
	free(sresp);
      return -1;
    }
    free(sresp);
    logger.msg(DEBUG, "Data channel: %d.%d.%d.%d %d", pasv_addr.host[0],
	       pasv_addr.host[1], pasv_addr.host[2], pasv_addr.host[3],
	       pasv_addr.port);
    if (!(res = globus_ftp_control_local_port(handle, &pasv_addr))) {
      logger.msg(INFO, "Obtained host and address are not acceptable");
      logger.msg(INFO, "Failure: %s", res.str());
      return -1;
    }
    return 0;
  }

  int Lister::retrieve_dir(const URL& url) {
    GlobusResult res;
    /* get listing */
    fnames.clear();
    globus_ftp_control_auth_info_t auth;

    if ((url.Protocol() != "ftp") &&
	(url.Protocol() != "gsiftp")) {
      logger.msg(ERROR, "Unsupported protocol in url %s", url.str());
      return -1;
    }

    bool reconnect = true;

    if (connected)
      if ((host == url.Host()) &&
	  (port == url.Port()) &&
	  (scheme == url.Protocol()) &&
	  (username == url.Username()) &&
	  (userpass == url.Passwd())) {
	/* same server - check if connection alive */
	logger.msg(DEBUG, "Reusing connection");
	if (send_command("NOOP", NULL, true, NULL) ==
	    GLOBUS_FTP_POSITIVE_COMPLETION_REPLY)
	  reconnect = false;
      }

    path = '/' + url.Path();
    if ((path.length() != 0) && (path[path.length() - 1] == '/'))
      path.resize(path.length() - 1);
    if (reconnect) {
      connected = false;
      port = url.Port();
      scheme = url.Protocol();
      host = url.Host();
      username = url.Username();
      userpass = url.Passwd();
      /*
	 !!!!!!!!!!!!!!!!!!!!!!!!!!!
	 disconnect here ???????????
       */
      if (!(res = globus_ftp_control_connect(handle,
					     const_cast<char*>(host.c_str()),
					     port, &resp_callback, this))) {
	logger.msg(ERROR, "Failed connecting to server %s:%d",
		   host.c_str(), port);
	logger.msg(ERROR, "Failure: %s", res.str());
	return -1;
      }
      if (wait_for_callback() != CALLBACK_DONE) {
	logger.msg(ERROR, "Failed to connect to server %s:%d",
		   host.c_str(), port);
	resp_destroy();
	return -1;
      }
      resp_destroy();
      char *username_ = const_cast<char*>(username.c_str());
      char *userpass_ = const_cast<char*>(userpass.c_str());
      globus_bool_t use_auth;
      if (scheme == "gsiftp") {
	if (username.empty())
	  username_ = default_gsiftp_user;
	if (userpass.empty())
	  userpass_ = default_gsiftp_pass;
	if (globus_ftp_control_auth_info_init(&auth, credential,
					      GLOBUS_TRUE, username_,
					      userpass_, GLOBUS_NULL,
					      GLOBUS_NULL) !=
	    GLOBUS_SUCCESS) {
	  logger.msg(ERROR, "Bad authentication information");
	  return -1;
	}
	use_auth = GLOBUS_TRUE;
      }
      else {
	if (username.empty())
	  username_ = default_ftp_user;
	if (userpass.empty())
	  userpass_ = default_ftp_pass;
	if (globus_ftp_control_auth_info_init(&auth, GSS_C_NO_CREDENTIAL,
					      GLOBUS_FALSE, username_,
					      userpass_, GLOBUS_NULL,
					      GLOBUS_NULL) !=
	    GLOBUS_SUCCESS) {
	  logger.msg(ERROR, "Bad authentication information");
	  return -1;
	}
	use_auth = GLOBUS_FALSE;
      }
      if (globus_ftp_control_authenticate(handle, &auth, use_auth,
					  resp_callback, this) !=
	  GLOBUS_SUCCESS) {
	logger.msg(ERROR, "Failed authenticating");
	return -1;
      }
      if (wait_for_callback() != CALLBACK_DONE) {
	logger.msg(ERROR, "Failed authenticating");
	resp_destroy();
	return -1;
      }
      resp_destroy();
      connected = true;
    }
    globus_ftp_control_response_class_t cmd_resp;
    char *sresp;
    if (url.Protocol() == "gsiftp") {
      cmd_resp = send_command("DCAU", "N", true, &sresp, '"');
      if ((cmd_resp != GLOBUS_FTP_POSITIVE_COMPLETION_REPLY) &&
	  (cmd_resp != GLOBUS_FTP_PERMANENT_NEGATIVE_COMPLETION_REPLY)) {
	if (sresp) {
	  logger.msg(INFO, "DCAU failed: %s", sresp);
	  free(sresp);
	}
	else
	  logger.msg(INFO, "DCAU failed");
	return -1;
      }
      free(sresp);
    }
    globus_ftp_control_dcau_t dcau;
    dcau.mode = GLOBUS_FTP_CONTROL_DCAU_NONE;
    globus_ftp_control_local_dcau(handle, &dcau, GSS_C_NO_CREDENTIAL);
    globus_ftp_control_host_port_t pasv_addr;
    /* try MLSD */
    facts = true;
    if (setup_pasv(pasv_addr) != 0)
      return -1;
    /* it looks like _pasv is not enough for connection - start reading
       immediately */
    data_callback_status = (callback_status_t)CALLBACK_NOTREADY;
    if (globus_ftp_control_data_connect_read(handle, &list_conn_callback,
					     this) != GLOBUS_SUCCESS) {
      logger.msg(INFO, "Failed to open data channel");
      return -1;
    }
    cmd_resp = send_command("MLSD", path.c_str(), true, &sresp);
    if (cmd_resp == GLOBUS_FTP_PERMANENT_NEGATIVE_COMPLETION_REPLY) {
      logger.msg(INFO, "MLSD is not supported - trying NLST");
      /* run NLST */
      facts = false;
      cmd_resp = send_command("NLST", path.c_str(), true, &sresp);
    }
    if (cmd_resp == GLOBUS_FTP_POSITIVE_COMPLETION_REPLY) {
      /* completion is not expected here */
      logger.msg(INFO, "Immediate completion: %s", sresp);
      if (sresp)
	free(sresp);
      return -1;
    }
    if ((cmd_resp != GLOBUS_FTP_POSITIVE_PRELIMINARY_REPLY) &&
	(cmd_resp != GLOBUS_FTP_POSITIVE_INTERMEDIATE_REPLY)) {
      if (sresp) {
	logger.msg(INFO, "NLST/MLSD failed: %s", sresp);
	free(sresp);
      }
      else
	logger.msg(INFO, "NLST/MLSD failed");
      return -1;
    }
    free(sresp);
    /* start transfer */
    for (;;) {
      /* waiting for response received */
      cmd_resp = send_command(NULL, NULL, true, &sresp);
      if (cmd_resp == GLOBUS_FTP_POSITIVE_COMPLETION_REPLY)
	break;
      if ((cmd_resp != GLOBUS_FTP_POSITIVE_PRELIMINARY_REPLY) &&
	  (cmd_resp != GLOBUS_FTP_POSITIVE_INTERMEDIATE_REPLY)) {
	if (sresp) {
	  logger.msg(INFO, "Data transfer aborted: %s", sresp);
	  free(sresp);
	}
	else
	  logger.msg(INFO, "Data transfer aborted");
	// Destroy data connections here ?????????
	return -1;
      }
      if (sresp)
	free(sresp);
    }
    if (sresp)
      free(sresp);
    /* waiting for data ended */
    if (wait_for_data_callback() != CALLBACK_DONE) {
      logger.msg(INFO, "Failed to transfer data");
      return -1;
    }
    /* success */
    return 0;
  }

} // namespace Arc
