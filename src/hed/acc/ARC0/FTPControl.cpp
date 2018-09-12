// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/Logger.h>
#include <arc/Thread.h>
#include <arc/crypto/OpenSSL.h>
#include <arc/globusutils/GlobusErrorUtils.h>
#include <arc/globusutils/GSSCredential.h>
#include <arc/globusutils/GlobusWorkarounds.h>

#include "FTPControl.h"

namespace Arc {

  static bool activated_ = false;

  class FTPControl::CBArg {
   public:
    Arc::SimpleCondition cond;
    std::string response;
    bool responseok;
    bool data;
    bool ctrl;
    bool close;
    CBArg(void);
    std::string Response();
  };

  FTPControl::CBArg::CBArg(void): responseok(true), data(true), ctrl(true), close(true) {}

  std::string FTPControl::CBArg::Response() {
    cond.lock();
    std::string res = response;
    cond.unlock();
    return res;
  }

  static void CloseCallback(void *arg, globus_ftp_control_handle_t*,
                            globus_object_t* /* error */,
                            globus_ftp_control_response_t* /* response */) {
    FTPControl::CBArg *cb = (FTPControl::CBArg*)arg;
    // TODO: handle error - if ever can happen here
    cb->close = true;
    cb->cond.signal();
  }

  static void ControlCallback(void *arg, globus_ftp_control_handle_t*,
                              globus_object_t *error,
                              globus_ftp_control_response_t *response) {
    FTPControl::CBArg *cb = (FTPControl::CBArg*)arg;
    if (error != GLOBUS_SUCCESS) {
      cb->response = Arc::globus_object_to_string(error);
      cb->responseok = false;
    }
    if (response && response->response_buffer) {
      int len = response->response_length;
      while (len > 0 && (response->response_buffer[len - 1] == '\r' ||
                         response->response_buffer[len - 1] == '\n' ||
                         response->response_buffer[len - 1] == '\0'))
        len--;
      cb->cond.lock();
      cb->response.assign((const char*)response->response_buffer, len);
      switch (response->response_class) {
      case GLOBUS_FTP_POSITIVE_PRELIMINARY_REPLY:
      case GLOBUS_FTP_POSITIVE_COMPLETION_REPLY:
      case GLOBUS_FTP_POSITIVE_INTERMEDIATE_REPLY:
        cb->responseok = true;
        break;

      default:
        cb->responseok = false;
        break;
      }
      cb->cond.unlock();
    }
    cb->ctrl = true;
    cb->cond.signal();
  }

  static void DataCloseCallback(void *arg, globus_ftp_control_handle_t*,
                              globus_object_t *error) {
    ControlCallback(arg, NULL, error, NULL);
  }

  static void ConnectCallback(void *arg, globus_ftp_control_handle_t*,
                              globus_object_t *error,
                              globus_ftp_control_response_t *response) {
    ControlCallback(arg,NULL,error,response);
  }

  static void DataConnectCallback(void *arg, globus_ftp_control_handle_t*,
                              unsigned int, globus_bool_t, globus_object_t*) {
    FTPControl::CBArg *cb = (FTPControl::CBArg*)arg;
    cb->data = true;
    cb->cond.signal();
  }

  static void ReadWriteCallback(void *arg, globus_ftp_control_handle_t*,
                                globus_object_t*, globus_byte_t*, globus_size_t,
                                globus_off_t, globus_bool_t) {
    FTPControl::CBArg *cb = (FTPControl::CBArg*)arg;
    cb->data = true;
    cb->cond.signal();
  }

  Logger FTPControl::logger(Logger::getRootLogger(), "FTPControl");

  FTPControl::FTPControl() {
    connected = false;
    cb = new CBArg;
    if(!activated_) {
      OpenSSLInit();
#ifdef HAVE_GLOBUS_THREAD_SET_MODEL
      globus_thread_set_model("pthread");
#endif
      GlobusPrepareGSSAPI();
      GlobusModuleActivate(GLOBUS_FTP_CONTROL_MODULE);
      activated_ = GlobusRecoverProxyOpenSSL();
    }
  }

  FTPControl::~FTPControl() {
    Disconnect(10);
    // Not deactivating Globus - that may be dangerous in some cases.
    // Deactivation also not needeed because this plugin is persistent.
    //globus_module_deactivate(GLOBUS_FTP_CONTROL_MODULE);
    delete cb;
  }

  bool FTPControl::Connect(const URL& url,
                           const UserConfig& uc) {

    int timeout = uc.Timeout();
    bool timedin;
    GlobusResult result;

    result = globus_ftp_control_handle_init(&control_handle);
    if (!result) {
      logger.msg(VERBOSE, "Connect: Failed to init handle: %s", result.str());
      return false;
    }

    result = globus_ftp_control_ipv6_allow(&control_handle, GLOBUS_TRUE);
    if (!result) {
      logger.msg(VERBOSE, "Failed to enable IPv6: %s", result.str());
    }

    cb->ctrl = false;
    connected = true;
    result = globus_ftp_control_connect(&control_handle,
                                        const_cast<char*>(url.Host().c_str()),
                                        url.Port(), &ConnectCallback,
                                        cb);
    if (!result) {
      logger.msg(VERBOSE, "Connect: Failed to connect: %s", result.str());
      connected = false;
      return false;
    }
    while (!cb->ctrl) {
      timedin = cb->cond.wait(timeout * 1000);
      if (!timedin) {
        logger.msg(VERBOSE, "Connect: Connecting timed out after %d ms",
                   timeout * 1000);
        Disconnect(timeout);
        return false;
      }
    }
    if (!cb->responseok) {
      logger.msg(VERBOSE, "Connect: Failed to connect: %s", cb->Response());
      Disconnect(timeout);
      return false;
    }

    GSSCredential handle(uc);

    globus_ftp_control_auth_info_t auth;
    result = globus_ftp_control_auth_info_init(&auth, handle, GLOBUS_TRUE,
                                               const_cast<char*>("ftp"),
                                               const_cast<char*>("user@"),
                                               GLOBUS_NULL, GLOBUS_NULL);
    if (!result) {
      logger.msg(VERBOSE, "Connect: Failed to init auth info handle: %s",
                 result.str());
      Disconnect(timeout);
      return false;
    }

    cb->ctrl = false;
    result = globus_ftp_control_authenticate(&control_handle, &auth,
                                             GLOBUS_TRUE,
                                             &ControlCallback, cb);
    if (!result) {
      logger.msg(VERBOSE, "Connect: Failed authentication: %s", result.str());
      Disconnect(timeout);
      return false;
    }
    while (!cb->ctrl) {
      timedin = cb->cond.wait(timeout * 1000);
      if (!timedin) {
        logger.msg(VERBOSE, "Connect: Authentication timed out after %d ms",
                   timeout * 1000);
        Disconnect(timeout);
        return false;
      }
    }
    if (!cb->responseok) {
      logger.msg(VERBOSE, "Connect: Failed authentication: %s", cb->Response());
      Disconnect(timeout);
      return false;
    }

    return true;

  } // end Connect

  bool FTPControl::SendCommand(const std::string& cmd, int timeout) {

    bool timedin;
    GlobusResult result;

    logger.msg(DEBUG, "SendCommand: Command: %s", cmd);
    cb->ctrl = false;
    result = globus_ftp_control_send_command(&control_handle, cmd.c_str(),
                                             &ControlCallback, cb);
    if (!result) {
      logger.msg(VERBOSE, "SendCommand: Failed: %s", result.str());
      return false;
    }
    while (!cb->ctrl) {
      timedin = cb->cond.wait(timeout * 1000);
      if (!timedin) {
        logger.msg(VERBOSE, "SendCommand: Timed out after %d ms", timeout * 1000);
        return false;
      }
    }
    if (!cb->responseok) {
      logger.msg(VERBOSE, "SendCommand: Failed: %s", cb->Response());
      return false;
    }
    logger.msg(DEBUG, "SendCommand: Response: %s", cb->Response());

    return true;

  } // end SendCommand

  bool FTPControl::SendCommand(const std::string& cmd, std::string& response,
                               int timeout) {

    bool timedin;
    GlobusResult result;

    cb->ctrl = false;
    logger.msg(DEBUG, "SendCommand: Command: %s", cmd);
    result = globus_ftp_control_send_command(&control_handle, cmd.c_str(),
                                             &ControlCallback, cb);
    if (!result) {
      logger.msg(VERBOSE, "SendCommand: Failed: %s", result.str());
      return false;
    }
    while (!cb->ctrl) {
      timedin = cb->cond.wait(timeout * 1000);
      if (!timedin) {
        logger.msg(VERBOSE, "SendCommand: Timed out after %d ms", timeout * 1000);
        return false;
      }
    }
    if (!cb->responseok) {
      logger.msg(VERBOSE, "SendCommand: Failed: %s", cb->Response());
      return false;
    }

    response = cb->Response();
    logger.msg(VERBOSE, "SendCommand: Response: %s", response);

    return true;

  } // end SendCommand


  bool FTPControl::SetupPASV(int timeout) {
    GlobusResult result;
    std::string response;
    globus_ftp_control_host_port_t passive_addr;
    passive_addr.port = 0;
    passive_addr.hostlen = 0;
    // Try EPSV first to make it work over IPv6
    if (!SendCommand("EPSV", response, timeout)) {
      // Now try PASV. It will fail on IPv6 unless server provides IPv4 data channel.
      if (!SendCommand("PASV", response, timeout)) {
        logger.msg(VERBOSE, "FTP Job Control: Failed sending EPSV and PASV commands");
        return false;
      }
      std::string::size_type pos1 = response.find('(');
      if (pos1 == std::string::npos) {
        logger.msg(VERBOSE, "FTP Job Control: Server PASV response parsing failed: %s",
                   response);
        return false;
      }
      std::string::size_type pos2 = response.find(')', pos1 + 1);
      if (pos2 == std::string::npos) {
        logger.msg(VERBOSE, "FTP Job Control: Server PASV response parsing failed: %s",
                   response);
        return false;
      }
      unsigned short port_low, port_high;
      if (sscanf(response.substr(pos1 + 1, pos2 - pos1 - 1).c_str(),
                 "%i,%i,%i,%i,%hu,%hu",
                 &passive_addr.host[0],
                 &passive_addr.host[1],
                 &passive_addr.host[2],
                 &passive_addr.host[3],
                 &port_high,
                 &port_low) == 6) {
        passive_addr.port = 256 * port_high + port_low;
        passive_addr.hostlen = 4;
      } else {
        logger.msg(VERBOSE, "FTP Job Control: Server PASV response parsing failed: %s",
                   response);
        return false;
      }
    } else {
      // Successful EPSV - response is (|||port|)
      // Currently more complex responses with protocol and host
      // are not supported.
      std::string::size_type pos1 = response.find('(');
      if (pos1 == std::string::npos) {
        logger.msg(VERBOSE, "FTP Job Control: Server EPSV response parsing failed: %s",
                   response);
        return false;
      }
      std::string::size_type pos2 = response.find(')', pos1 + 1);
      if (pos2 == std::string::npos) {
        logger.msg(VERBOSE, "FTP Job Control: Server EPSV response parsing failed: %s",
                   response);
        return false;
      }
      std::string sresp = response.substr(pos1 + 1, pos2 - pos1 - 1);
      char sep = sresp[0];
      if(!sep) {
        logger.msg(VERBOSE, "FTP Job Control: Server EPSV response parsing failed: %s",
                   response);
        return false;
      }
      char* lsep = NULL;
      if((sresp[1] != sep) || (sresp[2] != sep) ||
         ((lsep = (char*)strchr(sresp.c_str()+3,sep)) == NULL)) {
        logger.msg(VERBOSE, "FTP Job Control: Server EPSV response parsing failed: %s",
                   response);
        return false;
      }
      *lsep = 0;
      passive_addr.port = strtoul(sresp.c_str()+3,&lsep,10);
      if(passive_addr.port == 0) {
        logger.msg(VERBOSE, "FTP Job Control: Server EPSV response port parsing failed: %s",
                   response);
        return false;
      }
      // Apply control connection address
      unsigned short local_port;
      if(!(result = globus_io_tcp_get_remote_address_ex(&(control_handle.cc_handle.io_handle),
                                     passive_addr.host,&passive_addr.hostlen,&local_port))) {
        std::string globus_err(result.str());
        logger.msg(VERBOSE, "FTP Job Control:: Failed to apply local address to data connection: %s",
                   globus_err);
        return false;
      }
    }
    if (passive_addr.hostlen == 0) {
      logger.msg(VERBOSE, "FTP Job Control: Can't parse host and/or port in response to EPSV/PASV: %s",
                 response);
      return false;
    }
    if (passive_addr.hostlen == 4) {
      logger.msg(VERBOSE, "FTP Job Control: Data channel: %d.%d.%d.%d:%d",
                 passive_addr.host[0], passive_addr.host[1],
                 passive_addr.host[2], passive_addr.host[3],
                 passive_addr.port);
    } else {
      char buf[8*5];
      snprintf(buf,sizeof(buf),"%04x:%04x:%04x:%04x:%04x:%04x:%04x:%04x",
                 passive_addr.host[0]<<8  | passive_addr.host[1],
                 passive_addr.host[2]<<8  | passive_addr.host[3],
                 passive_addr.host[4]<<8  | passive_addr.host[5],
                 passive_addr.host[6]<<8  | passive_addr.host[7],
                 passive_addr.host[8]<<8  | passive_addr.host[9],
                 passive_addr.host[10]<<8 | passive_addr.host[11],
                 passive_addr.host[12]<<8 | passive_addr.host[13],
                 passive_addr.host[14]<<8 | passive_addr.host[15]);
      buf[sizeof(buf)-1] = 0;
      logger.msg(VERBOSE, "FTP Job Control: Data channel: [%s]:%d",
                 buf, passive_addr.port);
    }
    result = globus_ftp_control_local_port(&control_handle, &passive_addr);
    if (!result) {
      logger.msg(VERBOSE, "FTP Job Control: Local port failed: %s", result.str());
      return false;
    }
    /* it looks like _pasv is not enough for connection - start reading
       immediately 
    data_callback_status = (callback_status_t)CALLBACK_NOTREADY;
    if (globus_ftp_control_data_connect_read(handle, &list_conn_callback,
                                             this) != GLOBUS_SUCCESS) {
      logger.msg(INFO, "Failed to open data channel");
      result.SetDesc("Failed to open data channel to "+urlstr);
      pasv_set = false;
      return result;
    }
    */
    return true;
  }

  bool FTPControl::SendData(const std::string& data,
                            const std::string& filename, int timeout) {

    bool timedin;
    GlobusResult result;

    if (!SendCommand("DCAU N", timeout)) {
      logger.msg(INFO, "FTP Job Control: Failed sending DCAU command");
      return false;
    }

    if (!SendCommand("TYPE I", timeout)) {
      logger.msg(INFO, "FTP Job Control: Failed sending TYPE command");
      return false;
    }

    if(!SetupPASV(timeout)) return false;

    result = globus_ftp_control_local_type(&control_handle,
                                           GLOBUS_FTP_CONTROL_TYPE_IMAGE, 0);
    if (!result) {
      logger.msg(VERBOSE, "FTP Job Control: Local type failed: %s", result.str());
      return false;
    }

    cb->ctrl = false;
    cb->data = false;
    result = globus_ftp_control_send_command(&control_handle,
                                             ("STOR " + filename).c_str(),
                                             &ControlCallback, cb);
    if (!result) {
      logger.msg(INFO, "FTP Job Control: Failed sending STOR command: %s",
                 result.str());
      return false;
    }

    result = globus_ftp_control_data_connect_write(&control_handle,
                                                   &DataConnectCallback, cb);
    if (!result) {
      logger.msg(INFO, "FTP Job Control: Data connect write failed: %s",
                 result.str());
      return false;
    }
    while (!cb->data) {
      timedin = cb->cond.wait(timeout * 1000);
      if (!timedin) {
        logger.msg(INFO, "FTP Job Control: Data connect write timed out after %d ms",
                   timeout * 1000);
        return false;
      }
    }
    while (!cb->ctrl) {
      timedin = cb->cond.wait(timeout * 1000);
      if (!timedin) {
        logger.msg(INFO, "FTP Job Control: Data connect write timed out after %d ms",
                   timeout * 1000);
        return false;
      }
    }
    if (!cb->responseok) {
      logger.msg(INFO, "FTP Job Control: Data connect write failed: %s",
                 cb->Response());
      return false;
    }

    cb->data = false;
    cb->ctrl = false;
    result = globus_ftp_control_data_write(&control_handle,
                                           (globus_byte_t*)data.c_str(),
                                           data.size(), 0, GLOBUS_TRUE,
                                           &ReadWriteCallback, cb);
    if (!result) {
      logger.msg(INFO, "FTP Job Control: Data write failed: %s", result.str());
      return false;
    }
    while (!cb->data) {
      timedin = cb->cond.wait(timeout * 1000);
      if (!timedin) {
        logger.msg(INFO, "FTP Job Control: Data write timed out after %d ms",
                   timeout * 1000);
        return false;
      }
    }
    while (!cb->ctrl) {
      timedin = cb->cond.wait(timeout * 1000);
      if (!timedin) {
        logger.msg(INFO, "FTP Job Control: Data write timed out after %d ms",
                   timeout * 1000);
        return false;
      }
    }
    if (!cb->responseok) {
      logger.msg(INFO, "FTP Job Control: Data write failed: %s", cb->Response());
      return false;
    }

    return true;

  } // end SendData

  bool FTPControl::Disconnect(int timeout) {
    if(!connected) return true;
    connected = false;

    GlobusResult result;

    // Do all posible to stop communication

    cb->ctrl = false;
    result = globus_ftp_control_data_force_close(&control_handle, &DataCloseCallback, cb);
    if (!result) {
      // Maybe connection is already lost
      logger.msg(VERBOSE, "Disconnect: Failed aborting - ignoring: %s", result.str());
    } else while (!cb->ctrl) {
      if(!cb->cond.wait(timeout * 1000)) {
        logger.msg(VERBOSE, "Disconnect: Data close timed out after %d ms", timeout * 1000);
      }
    }

    cb->ctrl = false;
    result = globus_ftp_control_abort(&control_handle, &ControlCallback, cb);
    if (!result) {
      // Maybe connection is already lost
      logger.msg(VERBOSE, "Disconnect: Failed aborting - ignoring: %s", result.str());
    } else while (!cb->ctrl) {
      if(!cb->cond.wait(timeout * 1000)) {
        logger.msg(VERBOSE, "Disconnect: Abort timed out after %d ms", timeout * 1000);
      }
    }

    cb->ctrl = false;
    result = globus_ftp_control_quit(&control_handle, &ControlCallback, cb);
    if (!result) {
      // Maybe connection is already lost
      logger.msg(VERBOSE, "Disconnect: Failed quitting - ignoring: %s", result.str());
    } else while (!cb->ctrl) {
      if(!cb->cond.wait(timeout * 1000)) {
        logger.msg(VERBOSE, "Disconnect: Quitting timed out after %d ms", timeout * 1000);
      }
    }

    cb->close = false;
    result = globus_ftp_control_force_close(&control_handle, &CloseCallback, cb);
    if (!result) {
      // Assuming only reason for failure here is that connection is 
      // already closed
      logger.msg(DEBUG, "Disconnect: Failed closing - ignoring: %s",
                 result.str());
    } else while (!cb->close) {
      // Need to wait for callback to make sure handle destruction will work
      // Hopefully forced close should never take long time
      if(!cb->cond.wait(timeout * 1000)) {
        logger.msg(VERBOSE, "Disconnect: Closing timed out after %d ms", timeout * 1000);
      }
    }

    bool first_time = true;
    time_t start_time = time(NULL);
    // Waiting for stalled callbacks
    // If globus_ftp_control_handle_destroy is called with dc_handle
    // state not GLOBUS_FTP_DATA_STATE_NONE then handle is messed
    // and next call causes assertion. So here we are waiting for
    // proper state.
    globus_mutex_lock(&(control_handle.cc_handle.mutex));
    while ((control_handle.dc_handle.state != GLOBUS_FTP_DATA_STATE_NONE) ||
           (control_handle.cc_handle.cc_state != GLOBUS_FTP_CONTROL_UNCONNECTED)) {
      if(first_time) {
        logger.msg(VERBOSE, "Disconnect: waiting for globus handle to settle");
        first_time = false;
      }
      //if((control_handle.cc_handle.cc_state == GLOBUS_FTP_CONTROL_UNCONNECTED) &&
      //   (control_handle.dc_handle.state == GLOBUS_FTP_DATA_STATE_CLOSING)) {
      //  logger.msg(VERBOSE, "Disconnect: Tweaking Globus to think data connection is closed");
      //  control_handle.dc_handle.state = GLOBUS_FTP_DATA_STATE_NONE;
      //  break;
      //}
      globus_mutex_unlock(&(control_handle.cc_handle.mutex));
      cb->cond.wait(1000);
      globus_mutex_lock(&(control_handle.cc_handle.mutex));
      // Protection against broken Globus
      if(((unsigned int)(time(NULL)-start_time)) > 60) {
        logger.msg(VERBOSE, "Disconnect: globus handle is stuck.");
        break;
      }
    }
    globus_mutex_unlock(&(control_handle.cc_handle.mutex));
    if(!(result = globus_ftp_control_handle_destroy(&control_handle))) {
      // This situation can't be fixed because call to globus_ftp_control_handle_destroy
      // makes handle unusable even if it fails.
      logger.msg(VERBOSE, "Disconnect: Failed destroying handle: %s. Can't handle such situation.",result.str());
      cb = NULL;
    } else if(!first_time) {
      logger.msg(VERBOSE, "Disconnect: handle destroyed.");
    }
    return true;

  } // end Disconnect

} // namespace Arc
