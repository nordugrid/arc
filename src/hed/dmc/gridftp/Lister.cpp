// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <list>
#include <string>

#include <globus_common.h>
#include <globus_ftp_control.h>
#include <globus_object.h>

#include <arc/Thread.h>
#include <arc/DateTime.h>
#include <arc/Logger.h>
#include <arc/StringConv.h>
#include <arc/URL.h>
#include <arc/data/FileInfo.h>
#include <arc/globusutils/GlobusErrorUtils.h>
#include <arc/globusutils/GSSCredential.h>

#include "Lister.h"


static char default_ftp_user[] = "anonymous";
static char default_gsiftp_user[] = ":globus-mapping:";
static char default_ftp_pass[] = "dummy";
static char default_gsiftp_pass[] = "user@";

static void dos_to_unix(char *s) {
  if (!s)
    return;
  int l = strlen(s);
  for (; l;) {
    l--;
    if ((s[l] == '\r') || (s[l] == '\n'))
      s[l] = ' ';
  }
}


namespace ArcDMCGridFTP {

  using namespace Arc;


  static Logger logger(Logger::rootLogger, "Lister");


  std::map<void*,Lister*> Lister::callback_args;

  Glib::Mutex Lister::callback_args_mutex;

  void* Lister::remember_for_callback(Lister* it) {
    static void* last_arg = NULL;
    callback_args_mutex.lock();
    void* arg = last_arg;
    std::map<void*,Lister*>::iterator pos = callback_args.find(last_arg);
    if(pos != callback_args.end()) {
      // must be very old stuck communication - too old to keep
      globus_mutex_t* pos_mutex = &pos->second->mutex;
      globus_mutex_lock(pos_mutex);
      callback_args.erase(pos);      
      globus_mutex_unlock(pos_mutex);
    };
    callback_args[last_arg] = it;
    last_arg = (void*)(((unsigned long int)last_arg) + 1);
    callback_args_mutex.unlock();
    return arg;
  }

  Lister* Lister::recall_for_callback(void* arg) {
    callback_args_mutex.lock();
    Lister* it = NULL;
    std::map<void*,Lister*>::iterator pos = callback_args.find(arg);
    if(pos != callback_args.end()) {
      it = pos->second;
      globus_mutex_lock(&it->mutex);
    };
    callback_args_mutex.unlock();
    return it;
  }

  void Lister::forget_about_callback(void* arg) {
    callback_args_mutex.lock();
    std::map<void*,Lister*>::iterator pos = callback_args.find(arg);
    if(pos != callback_args.end()) {
      globus_mutex_t* pos_mutex = &pos->second->mutex;
      globus_mutex_lock(pos_mutex);
      callback_args.erase(pos);      
      globus_mutex_unlock(pos_mutex);
    };
    callback_args_mutex.unlock();
  }

  void SetAttributes(FileInfo& fi, const char *facts) {
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
          fi.SetModified(stringtoi(tmp_s)); // UNIX time
        else
          fi.SetModified(tmp_s); // ISO time
      }
    }
  }

  Lister::callback_status_t Lister::wait_for_callback(unsigned int to) {
    callback_status_t res;
    globus_mutex_lock(&mutex);
    globus_abstime_t timeout;
    GlobusTimeAbstimeSet(timeout,to,0);
    while (callback_status == CALLBACK_NOTREADY) {
      if (globus_cond_timedwait(&cond, &mutex, &timeout) == ETIMEDOUT) {
        callback_status = CALLBACK_NOTREADY;
        globus_mutex_unlock(&mutex);
        return CALLBACK_TIMEDOUT;
      }
    }
    res = callback_status;
    callback_status = CALLBACK_NOTREADY;
    globus_mutex_unlock(&mutex);
    return res;
  }

  Lister::callback_status_t Lister::wait_for_data_callback(unsigned int to) {
    callback_status_t res;
    globus_mutex_lock(&mutex);
    globus_abstime_t timeout;
    GlobusTimeAbstimeSet(timeout,to,0);
    while (data_callback_status == CALLBACK_NOTREADY) {
      if (globus_cond_timedwait(&cond, &mutex, &timeout) == ETIMEDOUT) {
        data_callback_status = CALLBACK_NOTREADY;
        globus_mutex_unlock(&mutex);
        return CALLBACK_TIMEDOUT;
      }
    }
    res = data_callback_status;
    data_callback_status = CALLBACK_NOTREADY;
    globus_mutex_unlock(&mutex);
    return res;
  }

  Lister::callback_status_t Lister::wait_for_close_callback(unsigned int to) {
    callback_status_t res;
    globus_mutex_lock(&mutex);
    globus_abstime_t timeout;
    GlobusTimeAbstimeSet(timeout,to,0);
    while (close_callback_status == CALLBACK_NOTREADY) {
      if (globus_cond_timedwait(&cond, &mutex, &timeout) == ETIMEDOUT) {
        close_callback_status = CALLBACK_NOTREADY;
        globus_mutex_unlock(&mutex);
        return CALLBACK_TIMEDOUT;
      }
    }
    res = close_callback_status;
    close_callback_status = CALLBACK_NOTREADY;
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
    Lister *it = recall_for_callback(arg);
    if(!it) return;
    Arc::Logger::getRootLogger().setThreadContext();
    Arc::Logger::getRootLogger().removeDestinations();
    if (error != GLOBUS_SUCCESS) {
      it->callback_status = CALLBACK_ERROR;
      logger.msg(INFO, "Failure: %s", globus_object_to_string(error));
      if (response)
        logger.msg(INFO, "Response: %s", response->response_buffer);
    }
    else {
      if (it->resp_n < LISTER_MAX_RESPONSES) {
        memmove((it->resp) + 1, it->resp,
                sizeof(globus_ftp_control_response_t) * (it->resp_n));
        if (response && response->response_buffer) {
          globus_ftp_control_response_copy(response, it->resp);
        } else {    // invalid reply causes *_copy to segfault
          it->resp->response_buffer = (globus_byte_t*)strdup("000 ");
          it->resp->response_buffer_size = 5;
          it->resp->response_length = 4;
          it->resp->code = 0;
          it->resp->response_class = GLOBUS_FTP_UNKNOWN_REPLY;
        }
        (it->resp_n)++;
      }
      it->callback_status = CALLBACK_DONE;
      if(response && response->response_buffer) {
        dos_to_unix((char*)(response->response_buffer));
        logger.msg(VERBOSE, "Response: %s", response->response_buffer);
      }
    }
    globus_cond_signal(&(it->cond));
    globus_mutex_unlock(&(it->mutex));
  }

  void Lister::close_callback(void *arg, globus_ftp_control_handle_t*,
                             globus_object_t *error,
                             globus_ftp_control_response_t *response) {
    Lister *it = recall_for_callback(arg);
    if(!it) return;
    Arc::Logger::getRootLogger().setThreadContext();
    Arc::Logger::getRootLogger().removeDestinations();
    if (error != GLOBUS_SUCCESS) {
      it->close_callback_status = CALLBACK_ERROR;
    } else {
      it->close_callback_status = CALLBACK_DONE;
    }
    globus_cond_signal(&(it->cond));
    globus_mutex_unlock(&(it->mutex));
    return;
  }

  void Lister::simple_callback(void *arg, globus_ftp_control_handle_t*,
                             globus_object_t *error) {
    resp_callback(arg, NULL, error, NULL);
  }

  void Lister::list_read_callback(void *arg,
                                  globus_ftp_control_handle_t*,
                                  globus_object_t *error,
                                  globus_byte_t*,
                                  globus_size_t length,
                                  globus_off_t,
                                  globus_bool_t eof) {
    Lister *it = recall_for_callback(arg);
    if(!it) return;
    if(!it->data_activated) {
      globus_mutex_unlock(&(it->mutex));
      return;
    }
    length += it->list_shift;
    if (error != GLOBUS_SUCCESS) {
      /* no such file or connection error - assume no such file */
      logger.msg(INFO, "Error getting list of files (in list)");
      logger.msg(INFO, "Failure: %s", globus_object_to_string(error));
      logger.msg(INFO, "Assuming - file not found");
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
      if ((*name) == 0) break;
      globus_size_t nlen;
      nlen = strcspn(name, "\n\r");
      name[nlen] = 0;
      logger.msg(VERBOSE, "list record: %s", name);
      if (nlen == length) {
        if (!eof) {
          memmove(it->readbuf, name, nlen);
          it->list_shift = nlen;
          break;
        }
      }
      if (nlen == 0) { // skip empty std::string
        if (length == 0) break;
        name++;
        length--;
        continue;
      }
      char *attrs = name;
      if (it->facts) {
        for (; *name;) {
          nlen--;
          length--;
          if (*name == ' ') {
            name++;
            break;
          }
          name++;
        }
      }
      if(it->free_format) {
        // assuming it is 'ls -l'-like
        // a lot of attributes followed by filename
        // NOTE: it is not possible to reliably distinguish files
        // with empty spaces. So assuming no such files.
        char* name_start = strrchr(name,' ');
        if(name_start) {
          nlen-=(name_start-name+1);
          length-=(name_start-name+1);
          name=name_start+1;
        };
      };
      std::list<FileInfo>::iterator i = it->fnames.insert(it->fnames.end(), FileInfo(name));

      if (it->facts) SetAttributes(*i, attrs);
      if (nlen == length) break;
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
                                       &list_read_callback, arg) != GLOBUS_SUCCESS) {
        logger.msg(INFO, "Failed reading list of files");
        it->data_callback_status = CALLBACK_ERROR;
        globus_cond_signal(&(it->cond));
      }
      globus_mutex_unlock(&(it->mutex));
      return;
    }
    it->data_activated = false;
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
    Lister *it = recall_for_callback(arg);
    if(!it) return;
    if (error != GLOBUS_SUCCESS) {
      logger.msg(INFO, "Failure: %s", globus_object_to_string(error));
      it->data_callback_status = CALLBACK_ERROR;
      // if data failed no reason to wait for control
      it->callback_status = CALLBACK_ERROR;
      globus_cond_signal(&(it->cond));
      globus_mutex_unlock(&(it->mutex));
      return;
    }
    it->list_shift = 0;
    it->fnames.clear();
    it->data_activated = true;
    if (globus_ftp_control_data_read(hctrl, (globus_byte_t*)(it->readbuf),
                                     sizeof(it->readbuf) - 1,
                                     &list_read_callback, arg) != GLOBUS_SUCCESS) {
      logger.msg(INFO, "Failed reading data");
      it->data_callback_status = CALLBACK_ERROR;
      // if data failed no reason to wait for control
      it->callback_status = CALLBACK_ERROR;
      globus_cond_signal(&(it->cond));
    }
    globus_mutex_unlock(&(it->mutex));
    return;
  }

  globus_ftp_control_response_class_t Lister::send_command(const char *command, const char *arg, bool wait_for_response, char **sresp, int* code, char delim) {
    char *cmd = NULL;
    if (sresp) (*sresp) = NULL;
    if (code) *code = 0;
    if (command) { /* if no command - waiting for second reply */
      globus_mutex_lock(&mutex);
      for (int i = 0; i < resp_n; i++) {
        globus_ftp_control_response_destroy(resp + i);
      }
      resp_n = 0;
      callback_status = CALLBACK_NOTREADY;
      globus_mutex_unlock(&mutex);
      {
        std::string cmds(command);
        if(arg) {
          cmds += " ";
          cmds += arg;
        }
        logger.msg(VERBOSE, "Command: %s", cmds);
        cmds += "\r\n";
        cmd = (char*)malloc(cmds.length()+1);
        if (cmd == NULL) {
          logger.msg(ERROR, "Memory allocation error");
          return GLOBUS_FTP_UNKNOWN_REPLY;
        }
        strncpy(cmd,cmds.c_str(),cmds.length()+1);
        cmd[cmds.length()] = 0;
      }
      if (globus_ftp_control_send_command(handle, cmd, resp_callback, callback_arg)
          != GLOBUS_SUCCESS) {
        logger.msg(VERBOSE, "%s failed", command);
        if (cmd) free(cmd);
        return GLOBUS_FTP_UNKNOWN_REPLY;
      }
      logger.msg(DEBUG, "Command is being sent");
    }
    if (wait_for_response) {
      globus_mutex_lock(&mutex);
      while ((callback_status == CALLBACK_NOTREADY) && (resp_n == 0)) {
        logger.msg(DEBUG, "Waiting for response");
        globus_cond_wait(&cond, &mutex);
      }
      free(cmd);
      if (callback_status != CALLBACK_DONE) {
        logger.msg(DEBUG, "Callback got failure");
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
            logger.msg(VERBOSE, "Response: %s", *sresp);
          }
          else
            logger.msg(ERROR, "Memory allocation error");
        }
        else {
          /* look for pair of enclosing characters */
          logger.msg(VERBOSE, "Response: %s", resp[resp_n - 1].response_buffer);
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
              logger.msg(VERBOSE, "Response: %s", *sresp);
            }
          }
        }
      }
      globus_ftp_control_response_class_t resp_class =
        GLOBUS_FTP_UNKNOWN_REPLY;
      int resp_code = 0;
      if (resp_n > 0) {
        resp_class = resp[resp_n - 1].response_class;
        resp_code = resp[resp_n - 1].code;
        globus_ftp_control_response_destroy(resp + (resp_n - 1));
        resp_n--;
      }
      if (resp_n == 0)
        callback_status = CALLBACK_NOTREADY;
      globus_mutex_unlock(&mutex);
      if (code) *code = resp_code;
      return resp_class;
    }
    else
      return GLOBUS_FTP_POSITIVE_COMPLETION_REPLY;
    /* !!!!!!! Memory LOST - cmd !!!!!!!! */
  }

  Lister::Lister()
    : inited(false),
      facts(true),
      handle(NULL),
      resp_n(0),
      callback_status(CALLBACK_NOTREADY),
      data_callback_status(CALLBACK_NOTREADY),
      close_callback_status(CALLBACK_NOTREADY),
      list_shift(0),
      connected(false),
      pasv_set(false),
      data_activated(false),
      free_format(false),
      port((unsigned short int)(-1)),
      credential(NULL) {
    if (globus_cond_init(&cond, GLOBUS_NULL) != GLOBUS_SUCCESS) {
      logger.msg(ERROR, "Failed in globus_cond_init");
      return;
    }
    if (globus_mutex_init(&mutex, GLOBUS_NULL) != GLOBUS_SUCCESS) {
      logger.msg(ERROR, "Failed in globus_mutex_init");
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
      logger.msg(ERROR, "Failed in globus_ftp_control_handle_init");
      globus_mutex_destroy(&mutex);
      globus_cond_destroy(&cond);
      free(handle);
      handle = NULL;
      return;
    }
    if (globus_ftp_control_ipv6_allow(handle,GLOBUS_TRUE)  != GLOBUS_SUCCESS) {
      logger.msg(WARNING, "Failed to enable IPv6");
    }
    callback_arg = remember_for_callback(this);
    inited = true;
  }

  void Lister::close_connection() {
    if (!connected) return;
    connected = false;
    bool res = true;
    close_callback_status = CALLBACK_NOTREADY;
    logger.msg(VERBOSE, "Closing connection");
    if (globus_ftp_control_data_force_close(handle, simple_callback, callback_arg) == GLOBUS_SUCCESS) {
      // Timeouts are used while waiting for callbacks just in case they never
      // come. If a timeout happens then the response object is not freed just
      // in case the callback eventually arrives.
      callback_status_t cbs = wait_for_callback(60);
      if (cbs == CALLBACK_TIMEDOUT) {
        logger.msg(VERBOSE, "Timeout waiting for Globus callback - leaking connection");
        return;
      }
      if (cbs != CALLBACK_DONE) res = false;
    }
    //if (globus_ftp_control_abort(handle, resp_callback, callback_arg) != GLOBUS_SUCCESS) {
    //} else if (wait_for_callback() != CALLBACK_DONE) {
    //    res = false;
    //}
    if(send_command("ABOR", NULL, true, NULL) == GLOBUS_FTP_UNKNOWN_REPLY) {
      res = false;
    }
    if (globus_ftp_control_quit(handle, resp_callback, callback_arg) == GLOBUS_SUCCESS) {
      callback_status_t cbs = wait_for_callback(60);
      if (cbs == CALLBACK_TIMEDOUT) {
        logger.msg(VERBOSE, "Timeout waiting for Globus callback - leaking connection");
        return;
      }
      if (cbs != CALLBACK_DONE) res = false;
    }
    if (globus_ftp_control_force_close(handle, close_callback, callback_arg) == GLOBUS_SUCCESS) {
      callback_status_t cbs = wait_for_close_callback();
      if (cbs != CALLBACK_DONE) res = false;
    }
    if (res) {
      logger.msg(VERBOSE, "Closed successfully");
    } else {
      logger.msg(VERBOSE, "Closing may have failed");
    }
    resp_destroy();
  }

  Lister::~Lister() {
    close_connection();
    if (inited) {
      inited = false;
      if(handle) {
        // Waiting for stalled callbacks
        // If globus_ftp_control_handle_destroy is called with dc_handle
        // state not GLOBUS_FTP_DATA_STATE_NONE then handle is messed
        // and next call causes assertion. So here we are waiting for
        // proper state.
        bool first_time = true;
        time_t start_time = time(NULL);
        globus_mutex_lock(&(handle->cc_handle.mutex));
        while ((handle->dc_handle.state != GLOBUS_FTP_DATA_STATE_NONE) ||
               (handle->cc_handle.cc_state != GLOBUS_FTP_CONTROL_UNCONNECTED)) {
//          if((handle->cc_handle.cc_state == GLOBUS_FTP_CONTROL_UNCONNECTED) &&
//             (handle->dc_handle.state == GLOBUS_FTP_DATA_STATE_CLOSING)) {
//            handle->dc_handle.state = GLOBUS_FTP_DATA_STATE_NONE;
//            break;
//          };
          globus_mutex_unlock(&(handle->cc_handle.mutex));
          if(first_time) {
            logger.msg(VERBOSE, "Waiting for globus handle to settle");
            first_time = false;
          }
          globus_abstime_t timeout;
          GlobusTimeAbstimeSet(timeout,0,100000);
          logger.msg(DEBUG, "Handle is not in proper state %u/%u",handle->cc_handle.cc_state,handle->dc_handle.state);
          globus_mutex_lock(&mutex);
          globus_cond_timedwait(&cond, &mutex, &timeout);
          globus_mutex_unlock(&mutex);
          globus_mutex_lock(&(handle->cc_handle.mutex));
          if(((unsigned int)(time(NULL) - start_time)) > 60) {
            logger.msg(VERBOSE, "Globus handle is stuck");
            first_time = false;
            break;
          }
        }
        // block callback execution in case anything left
        handle->cc_handle.close_cb_arg = NULL;
        handle->cc_handle.accept_cb_arg = NULL;
        handle->cc_handle.auth_cb_arg = NULL;
        handle->cc_handle.command_cb_arg = NULL;
        handle->dc_handle.close_callback_arg = NULL;
        globus_mutex_unlock(&(handle->cc_handle.mutex));
        GlobusResult res;
        if(!(res=globus_ftp_control_handle_destroy(handle))) {
          // This situation can't be fixed because call to globus_ftp_control_handle_destroy
          // makes handle unusable even if it fails.
          logger.msg(DEBUG, "Failed destroying handle: %s. Can't handle such situation.",res.str());
        } else {
          free(handle);
        };
        handle = NULL;
      };
      forget_about_callback(callback_arg);
      globus_mutex_destroy(&mutex);
      globus_cond_destroy(&cond);
    }
  }

  DataStatus Lister::setup_pasv(globus_ftp_control_host_port_t& pasv_addr) {
    if(pasv_set) return DataStatus::Success;
    char *sresp;
    GlobusResult res;
    DataStatus result = DataStatus::ListError;
    pasv_addr.port = 0;
    pasv_addr.hostlen = 0;
    // Try EPSV first to make it work over IPv6
    if (send_command("EPSV", NULL, true, &sresp, NULL, '(') !=
        GLOBUS_FTP_POSITIVE_COMPLETION_REPLY) {
      if (sresp) {
        logger.msg(INFO, "EPSV failed: %s", sresp);
        result.SetDesc("EPSV command failed at "+urlstr+" : "+sresp);
        free(sresp);
      } else {
        logger.msg(INFO, "EPSV failed");
        result.SetDesc("EPSV command failed at "+urlstr);
      }
      // Now try PASV. It will fail on IPv6 unless server provides IPv4 data channel.
      if (send_command("PASV", NULL, true, &sresp, NULL, '(') !=
          GLOBUS_FTP_POSITIVE_COMPLETION_REPLY) {
        if (sresp) {
          logger.msg(INFO, "PASV failed: %s", sresp);
          result.SetDesc("PASV command failed at "+urlstr+" : "+sresp);
          free(sresp);
        } else {
          logger.msg(INFO, "PASV failed");
          result.SetDesc("PASV command failed at "+urlstr);
        }
        return result;
      }
      if (sresp) {
        int port_low, port_high;
        if (sscanf(sresp, "%i,%i,%i,%i,%i,%i",
                   &(pasv_addr.host[0]), &(pasv_addr.host[1]),
                   &(pasv_addr.host[2]), &(pasv_addr.host[3]),
                   &port_high, &port_low) == 6) {
          pasv_addr.port = ((port_high & 0x000FF) << 8) | (port_low & 0x000FF);
          pasv_addr.hostlen = 4;
        }
        free(sresp);
      }
    } else {
      // Successful EPSV - response is (|||port|)
      // Currently more complex responses with protocol and host
      // are not supported.
      if (sresp) {
        char sep = sresp[0];
        char* lsep = NULL;
        if(sep) {
          if((sresp[1] == sep) && (sresp[2] == sep) &&
             ((lsep = (char*)strchr(sresp+3,sep)) != NULL)) {
            *lsep = 0;
            pasv_addr.port = strtoul(sresp+3,&lsep,10);
            if(pasv_addr.port != 0) {
              // Apply control connection address
              unsigned short local_port;
              if(!(res = globus_io_tcp_get_remote_address_ex(&(handle->cc_handle.io_handle),
                                     pasv_addr.host,&pasv_addr.hostlen,&local_port))) {
                logger.msg(INFO, "Failed to apply local address to data connection");
                std::string globus_err(res.str());
                logger.msg(INFO, "Failure: %s", globus_err);
                result.SetDesc("Failed to apply local address to data connection for "+urlstr+": "+globus_err);
                free(sresp);
                return result;
              }
            }
          }
        }
        free(sresp);
      }
    }
    if (pasv_addr.hostlen == 0) {
      logger.msg(INFO, "Can't parse host and/or port in response to EPSV/PASV");
      result.SetDesc("Can't parse host and/or port in response to EPSV/PASV from "+urlstr);
      return result;
    }
    if (pasv_addr.hostlen == 4) {
      logger.msg(VERBOSE, "Data channel: %d.%d.%d.%d:%d",
                 pasv_addr.host[0], pasv_addr.host[1], pasv_addr.host[2], pasv_addr.host[3],
                 pasv_addr.port);
    } else {
      char buf[8*5];
      snprintf(buf,sizeof(buf),"%04x:%04x:%04x:%04x:%04x:%04x:%04x:%04x",
                 pasv_addr.host[0]<<8  | pasv_addr.host[1],
                 pasv_addr.host[2]<<8  | pasv_addr.host[3],
                 pasv_addr.host[4]<<8  | pasv_addr.host[5],
                 pasv_addr.host[6]<<8  | pasv_addr.host[7],
                 pasv_addr.host[8]<<8  | pasv_addr.host[9],
                 pasv_addr.host[10]<<8 | pasv_addr.host[11],
                 pasv_addr.host[12]<<8 | pasv_addr.host[13],
                 pasv_addr.host[14]<<8 | pasv_addr.host[15]);
      buf[sizeof(buf)-1] = 0;
      logger.msg(VERBOSE, "Data channel: [%s]:%d",
                 buf, pasv_addr.port);
    }
    if (!(res = globus_ftp_control_local_port(handle, &pasv_addr))) {
      logger.msg(INFO, "Obtained host and address are not acceptable");
      std::string globus_err(res.str());
      logger.msg(INFO, "Failure: %s", globus_err);
      result.SetDesc("Host and address obtained from "+urlstr+" are not acceptable: "+globus_err);
      return result;
    }
    /* it looks like _pasv is not enough for connection - start reading
       immediately */
    data_callback_status = (callback_status_t)CALLBACK_NOTREADY;
    if (globus_ftp_control_data_connect_read(handle, &list_conn_callback, callback_arg) != GLOBUS_SUCCESS) {
      logger.msg(INFO, "Failed to open data channel");
      result.SetDesc("Failed to open data channel to "+urlstr);
      pasv_set = false;
      return result;
    }
    pasv_set = true;
    return DataStatus::Success;
  }

  DataStatus Lister::handle_connect(const URL& url) {
    GlobusResult res;
    DataStatus result = DataStatus::ListError;
    /* get listing */
    fnames.clear();
    globus_ftp_control_auth_info_t auth;

    if ((url.Protocol() != "ftp") &&
        (url.Protocol() != "gsiftp")) {
      logger.msg(VERBOSE, "Unsupported protocol in url %s", url.str());
      result.SetDesc("Unsupported protocol in url " + url.str());
      return result;
    }

    if (connected) {
      if ((host == url.Host()) &&
          (port == url.Port()) &&
          (scheme == url.Protocol()) &&
          (username == url.Username()) &&
          (userpass == url.Passwd())) {
        /* same server - check if connection alive */
        logger.msg(VERBOSE, "Reusing connection");
        if (send_command("NOOP", NULL, true, NULL) !=
            GLOBUS_FTP_POSITIVE_COMPLETION_REPLY) {
          // Connection failed, close now - we will connect again in the next step
          close_connection();
        }
      }
    }

    path = url.Path();
    urlstr = url.str();
    if ((path.length() != 0) && (path[path.length() - 1] == '/')) {
      path.resize(path.length() - 1);
    }
    if (!connected) {
      pasv_set = false;
      port = url.Port();
      scheme = url.Protocol();
      host = url.Host();
      username = url.Username();
      userpass = url.Passwd();
      if (!(res = globus_ftp_control_connect(handle,
                                             const_cast<char*>(host.c_str()),
                                             port, &resp_callback, callback_arg))) {
        logger.msg(VERBOSE, "Failed connecting to server %s:%d",
                   host.c_str(), port);
        result.SetDesc("Failed connecting to "+urlstr+" : "+res.str());
        return result;
      }
      if (wait_for_callback() != CALLBACK_DONE) {
        logger.msg(VERBOSE, "Failed to connect to server %s:%d",
                   host.c_str(), port);
        // TODO: error description from callback
        result.SetDesc("Failed to connect to server "+url.str());
        resp_destroy();
        return result;
      }
      connected = true;
      resp_destroy();
      char *username_ = const_cast<char*>(username.c_str());
      char *userpass_ = const_cast<char*>(userpass.c_str());
      globus_bool_t use_auth;
      if (scheme == "gsiftp") {
        if (username.empty()) username_ = default_gsiftp_user;
        if (userpass.empty()) userpass_ = default_gsiftp_pass;
        if (!credential) {
          logger.msg(VERBOSE, "Missing authentication information");
          result.SetDesc("Missing authentication information for "+url.str());
          return result;
        }
        if (!(res = globus_ftp_control_auth_info_init(&auth, *credential,
                                              GLOBUS_TRUE, username_,
                                              userpass_, GLOBUS_NULL,
                                              GLOBUS_NULL))) {
          std::string globus_err(res.str());
          logger.msg(VERBOSE, "Bad authentication information: %s", globus_err);
          result.SetDesc("Bad authentication information for "+urlstr+" : "+globus_err);
          return result;
        }
        use_auth = GLOBUS_TRUE;
      }
      else {
        if (username.empty()) username_ = default_ftp_user;
        if (userpass.empty()) userpass_ = default_ftp_pass;
        if (!(res = globus_ftp_control_auth_info_init(&auth, GSS_C_NO_CREDENTIAL,
                                              GLOBUS_FALSE, username_,
                                              userpass_, GLOBUS_NULL,
                                              GLOBUS_NULL))) {
          std::string globus_err(res.str());
          logger.msg(VERBOSE, "Bad authentication information: %s", globus_err);
          result.SetDesc("Bad authentication information for "+urlstr+" : "+globus_err);
          return result;
        }
        use_auth = GLOBUS_FALSE;
      }
      if (!(res = globus_ftp_control_authenticate(handle, &auth, use_auth,
                                          resp_callback, callback_arg))) {
        std::string globus_err(res.str());
        logger.msg(VERBOSE, "Failed authenticating: %s", globus_err);
        result.SetDesc("Failed authenticating at "+urlstr+" : "+globus_err);
        close_connection();
        return result;
      }
      if (wait_for_callback() != CALLBACK_DONE) {
        // TODO: error description from callback
        logger.msg(VERBOSE, "Failed authenticating");
        result.SetDesc("Failed authenticating at "+urlstr);
        resp_destroy();
        close_connection();
        return result;
      }
      for(int n = 0; n < resp_n; ++n) {
        if(resp[n].response_class != GLOBUS_FTP_POSITIVE_COMPLETION_REPLY) {
          logger.msg(VERBOSE, "Failed authenticating: %s", resp[n].response_buffer);
          result.SetDesc("Failed authenticating at "+urlstr+" : "+(char*)(resp[n].response_buffer));
          resp_destroy();
          close_connection();
          return result;
        }
      }
      resp_destroy();
    } else {
      // Calling NOOP to test connection
    }
    return DataStatus::Success;
  }

  DataStatus Lister::retrieve_file_info(const URL& url,bool names_only) {

    DataStatus result = DataStatus::StatError;
    DataStatus con_result = handle_connect(url);
    if(!con_result) return DataStatus(DataStatus::StatError, con_result.GetErrno(), con_result.GetDesc());

    globus_ftp_control_response_class_t cmd_resp;
    char *sresp = NULL;
    if (url.Protocol() == "gsiftp") {
      cmd_resp = send_command("DCAU", "N", true, &sresp, NULL, '"');
      if ((cmd_resp != GLOBUS_FTP_POSITIVE_COMPLETION_REPLY) &&
          (cmd_resp != GLOBUS_FTP_PERMANENT_NEGATIVE_COMPLETION_REPLY)) {
        if (sresp) {
          logger.msg(INFO, "DCAU failed: %s", sresp);
          result.SetDesc("DCAU command failed at "+urlstr+" : "+sresp);
          free(sresp); sresp = NULL;
        } else {
          logger.msg(INFO, "DCAU failed");
          result.SetDesc("DCAU command failed at "+urlstr);
        }
        return result;
      }
      free(sresp); sresp = NULL;
    }
    // default dcau
    globus_ftp_control_dcau_t dcau;
    dcau.mode = GLOBUS_FTP_CONTROL_DCAU_NONE;
    globus_ftp_control_local_dcau(handle, &dcau, GSS_C_NO_CREDENTIAL);
    globus_ftp_control_host_port_t pasv_addr;
    facts = true;
    free_format = false;
    if(!names_only) {
      /* try MLST */
      int code = 0;
      cmd_resp = send_command("MLST", path.c_str(), true, &sresp, &code);
      if (cmd_resp == GLOBUS_FTP_PERMANENT_NEGATIVE_COMPLETION_REPLY) {
        if (code == 500) {
          logger.msg(INFO, "MLST is not supported - trying LIST");
          free(sresp); sresp = NULL;
          /* run NLST */
          DataStatus pasv_res = setup_pasv(pasv_addr);
          if (!pasv_res) return pasv_res;
          facts = false;
          free_format = true;
          cmd_resp = send_command("LIST", path.c_str(), true, &sresp);
        }
      } else {
        // MLST replies through control channel
        // 250 -
        //  information
        // 250 -
        if (cmd_resp != GLOBUS_FTP_POSITIVE_COMPLETION_REPLY) {
          if(sresp) {
            logger.msg(INFO, "Immediate completion expected: %s", sresp);
            result.SetDesc("MLST command failed at "+urlstr+" : "+sresp);
            free(sresp); sresp = NULL;
          } else {
            logger.msg(INFO, "Immediate completion expected");
            result.SetDesc("MLST command failed at "+urlstr);
          }
          return result;
        }
        // Try to collect full response
        char* nresp = sresp?strchr(sresp,'\n'):NULL;
        if(nresp) {
          ++nresp;
        } else {
          free(sresp); sresp = NULL;
          cmd_resp = send_command(NULL, NULL, true, &sresp);
          if(cmd_resp != GLOBUS_FTP_UNKNOWN_REPLY) {
            logger.msg(INFO, "Missing information in reply: %s", sresp);
            if(sresp) {
              result.SetDesc("Missing information in reply from "+urlstr+" : "+sresp);
              free(sresp);
            } else {
              result.SetDesc("Missing information in reply from "+urlstr);
            }
            return result;
          }
          nresp=sresp;
        }
        char* fresp = NULL;
        if(nresp) {
          if(*nresp == ' ') ++nresp;
          fresp=strchr(nresp,'\n');
          if(fresp) {
            // callback
            *fresp=0;
            list_shift = 0;
            fnames.clear();
            size_t nlength = strlen(nresp);
            if(nlength > sizeof(readbuf)) nlength=sizeof(readbuf);
            memcpy(readbuf,nresp,nlength);
            data_activated = true;
            list_read_callback(callback_arg,handle,GLOBUS_SUCCESS,
                               (globus_byte_t*)readbuf,nlength,0,1);
          }
        };
        if(fresp) {
          ++fresp;
        } else {
          free(sresp); sresp = NULL;
          cmd_resp = send_command(NULL, NULL, true, &sresp);
          if(cmd_resp != GLOBUS_FTP_POSITIVE_COMPLETION_REPLY) {
            logger.msg(INFO, "Missing final reply: %s", sresp);
            if(sresp) {
              result.SetDesc("Missing final reply from "+urlstr+" : "+sresp);
              free(sresp);
            } else {
              result.SetDesc("Missing final reply from "+urlstr);
            }
            return result;
          }
          fresp=sresp;
        }
        free(sresp); sresp = NULL;
        return DataStatus::Success;
      }
    } else {
      DataStatus pasv_res = setup_pasv(pasv_addr);
      if (!pasv_res) return pasv_res;
      facts = false;
      free_format = true;
      cmd_resp = send_command("LIST", path.c_str(), true, &sresp);
    }
    if (cmd_resp == GLOBUS_FTP_POSITIVE_COMPLETION_REPLY) {
      /* completion is not expected here */
      pasv_set = false;
      logger.msg(INFO, "Unexpected immediate completion: %s", sresp);
      if(sresp) {
        result.SetDesc("Unexpected completion reply from "+urlstr+" : "+sresp);
        free(sresp); sresp = NULL;
      } else {
        result.SetDesc("Unexpected completion reply from "+urlstr);
      }
      return result;
    }
    if ((cmd_resp != GLOBUS_FTP_POSITIVE_PRELIMINARY_REPLY) &&
        (cmd_resp != GLOBUS_FTP_POSITIVE_INTERMEDIATE_REPLY)) {
      if (sresp) {
        logger.msg(INFO, "LIST/MLST failed: %s", sresp);
        result.SetDesc("LIST/MLST command failed at "+urlstr+" : "+sresp);
        result.SetErrno(globus_error_to_errno(sresp, result.GetErrno()));
        free(sresp); sresp = NULL;
      } else {
        logger.msg(INFO, "LIST/MLST failed");
        result.SetDesc("LIST/MLST command failed at "+urlstr);
      }
      return result;
    }
    free(sresp); sresp = NULL;
    result = transfer_list();
    if (!result) result = DataStatus(DataStatus::StatError, result.GetErrno(), result.GetDesc());
    return result;
  }

  DataStatus Lister::retrieve_dir_info(const URL& url,bool names_only) {

    DataStatus result = DataStatus::StatError;
    DataStatus con_result = handle_connect(url);
    if(!con_result) return con_result;

    globus_ftp_control_response_class_t cmd_resp;
    char *sresp = NULL;
    if (url.Protocol() == "gsiftp") {
      cmd_resp = send_command("DCAU", "N", true, &sresp, NULL, '"');
      if ((cmd_resp != GLOBUS_FTP_POSITIVE_COMPLETION_REPLY) &&
          (cmd_resp != GLOBUS_FTP_PERMANENT_NEGATIVE_COMPLETION_REPLY)) {
        if (sresp) {
          logger.msg(INFO, "DCAU failed: %s", sresp);
          result.SetDesc("DCAU command failed at "+urlstr+" : "+sresp);
          free(sresp);
        }
        else {
          logger.msg(INFO, "DCAU failed");
          result.SetDesc("DCAU command failed at "+urlstr);
        }
        return result;
      }
      free(sresp);
    }
    globus_ftp_control_dcau_t dcau;
    dcau.mode = GLOBUS_FTP_CONTROL_DCAU_NONE;
    globus_ftp_control_local_dcau(handle, &dcau, GSS_C_NO_CREDENTIAL);
    globus_ftp_control_host_port_t pasv_addr;
    facts = true;
    free_format = false;
    DataStatus pasv_res = setup_pasv(pasv_addr);
    if (!pasv_res) return pasv_res;
    if (!names_only) {
      /* try MLSD */
      int code = 0;
      cmd_resp = send_command("MLSD", path.c_str(), true, &sresp, &code);
      if (cmd_resp == GLOBUS_FTP_PERMANENT_NEGATIVE_COMPLETION_REPLY) {
        if (code == 500) {
          logger.msg(INFO, "MLSD is not supported - trying NLST");
          free(sresp);
          /* run NLST */
          facts = false;
          cmd_resp = send_command("NLST", path.c_str(), true, &sresp);
        }
      }
    } else {
      facts = false;
      cmd_resp = send_command("NLST", path.c_str(), true, &sresp);
    }
    if (cmd_resp == GLOBUS_FTP_POSITIVE_COMPLETION_REPLY) {
      /* completion is not expected here */
      pasv_set = false;
      logger.msg(INFO, "Immediate completion: %s", sresp?sresp:"");
      result.SetDesc("Unexpected completion response from "+urlstr+" : "+(sresp?sresp:""));
      if (sresp) free(sresp);
      return result;
    }
    if ((cmd_resp != GLOBUS_FTP_POSITIVE_PRELIMINARY_REPLY) &&
        (cmd_resp != GLOBUS_FTP_POSITIVE_INTERMEDIATE_REPLY)) {
      if (sresp) {
        logger.msg(INFO, "NLST/MLSD failed: %s", sresp);
        result.SetDesc("NLST/MLSD command failed at "+urlstr+" : "+sresp);
        result.SetErrno(globus_error_to_errno(sresp, result.GetErrno()));
        free(sresp);
      }
      else {
        logger.msg(INFO, "NLST/MLSD failed");
        result.SetDesc("NLST/MLSD command failed at "+urlstr);
      }
      return result;
    }
    free(sresp);
    return transfer_list();
  }

  DataStatus Lister::transfer_list(void) {
    DataStatus result = DataStatus::ListError;
    globus_ftp_control_response_class_t cmd_resp;
    char* sresp = NULL;
    /* start transfer */
    for (;;) {
      /* waiting for response received */
      cmd_resp = send_command(NULL, NULL, true, &sresp);
      if (cmd_resp == GLOBUS_FTP_POSITIVE_COMPLETION_REPLY) break;
      if ((cmd_resp != GLOBUS_FTP_POSITIVE_PRELIMINARY_REPLY) &&
          (cmd_resp != GLOBUS_FTP_POSITIVE_INTERMEDIATE_REPLY)) {
        if (sresp) {
          logger.msg(INFO, "Data transfer aborted: %s", sresp);
          result.SetDesc("Data transfer aborted at "+urlstr+" : "+sresp);
          free(sresp);
        }
        else {
          logger.msg(INFO, "Data transfer aborted");
          result.SetDesc("Data transfer aborted at "+urlstr);
        }
        // Destroy data connections here ?????????
        pasv_set = false;
        return result;
      }
      if (sresp) free(sresp);
    }
    if (sresp) free(sresp);
    /* waiting for data ended */
    if (wait_for_data_callback() != CALLBACK_DONE) {
      logger.msg(INFO, "Failed to transfer data");
      // TODO: error description from callback
      result.SetDesc("Failed to transfer data from "+urlstr);
      pasv_set = false;
      return result;
    }
    pasv_set = false;
    /* success */
    return DataStatus::Success;
  }

} // namespace ArcDMCGridFTP
