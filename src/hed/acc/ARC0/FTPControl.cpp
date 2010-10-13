// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/Logger.h>
#include <arc/Thread.h>
#include <arc/globusutils/GlobusErrorUtils.h>
#include <arc/globusutils/GSSCredential.h>

#include "FTPControl.h"

namespace Arc {

  class FTPControl::CBArg {
   private:
    unsigned int counter_;
    ~CBArg(void);
   public:
    Arc::SimpleCondition cond;
    std::string response;
    bool responseok;
    bool data;
    bool ctrl;
    bool close;
    CBArg(void);
    CBArg* claim(void);
    bool release(void);
  };

  FTPControl::CBArg::CBArg(void):counter_(1) {
  }

  FTPControl::CBArg::~CBArg(void) {
  }

  FTPControl::CBArg* FTPControl::CBArg::claim(void) {
    cond.lock();
    ++counter_;
    cond.unlock();
    return this;
  }

  bool FTPControl::CBArg::release(void) {
    cond.lock();
    --counter_;
    bool done = (counter_ <= 0);
    cond.unlock();
    // This looks unsafe, but is alright for how it is used
    if(done) delete this;
    return done;
  }

  static void CloseCallback(void *arg, globus_ftp_control_handle_t*,
                            globus_object_t* /* error */,
                            globus_ftp_control_response_t* /* response */) {
    FTPControl::CBArg *cb = (FTPControl::CBArg*)arg;
    // TODO: handle error - if ever can happen here
    cb->close = true;
    cb->cond.signal();
    cb->release();
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
    }
    cb->ctrl = true;
    cb->cond.signal();
  }

  static void ConnectCallback(void *arg, globus_ftp_control_handle_t*,
                              globus_object_t *error,
                              globus_ftp_control_response_t *response) {
    FTPControl::CBArg *cb = (FTPControl::CBArg*)arg;
    ControlCallback(arg,NULL,error,response);
    cb->release();
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
    globus_module_activate(GLOBUS_FTP_CONTROL_MODULE);
  }

  FTPControl::~FTPControl() {
    if(connected) Disconnect(10);
    globus_module_deactivate(GLOBUS_FTP_CONTROL_MODULE);
    cb->release();
  }

  bool FTPControl::Connect(const URL& url,
                           const std::string& proxyPath,
                           const std::string& certificatePath,
                           const std::string& keyPath,
                           int timeout) {

    bool timedin;
    GlobusResult result;

    result = globus_ftp_control_handle_init(&control_handle);
    if (!result) {
      logger.msg(VERBOSE, "Connect: Failed to init handle: %s", result.str());
      return false;
    }

    cb->ctrl = false;
    result = globus_ftp_control_connect(&control_handle,
                                        const_cast<char*>(url.Host().c_str()),
                                        url.Port(), &ConnectCallback,
                                        cb->claim());
    if (!result) {
      cb->release();
      logger.msg(VERBOSE, "Connect: Failed to connect: %s", result.str());
      return false;
    }
    while (!cb->ctrl) {
      timedin = cb->cond.wait(timeout * 1000);
      if (!timedin) {
        logger.msg(VERBOSE, "Connect: Connecting timed out after %d ms",
                   timeout * 1000);
        return false;
      }
    }
    if (!cb->responseok) {
      logger.msg(VERBOSE, "Connect: Failed to connect: %s", cb->response);
      return false;
    }
    connected = true;

    GSSCredential handle(proxyPath, certificatePath, keyPath);

    globus_ftp_control_auth_info_t auth;
    result = globus_ftp_control_auth_info_init(&auth, handle, GLOBUS_TRUE,
                                               const_cast<char*>("ftp"),
                                               const_cast<char*>("user@"),
                                               GLOBUS_NULL, GLOBUS_NULL);
    if (!result) {
      logger.msg(VERBOSE, "Connect: Failed to init auth info handle: %s",
                 result.str());
      return false;
    }

    cb->ctrl = false;
    result = globus_ftp_control_authenticate(&control_handle, &auth,
                                             GLOBUS_TRUE,
                                             &ControlCallback, cb);
    if (!result) {
      logger.msg(VERBOSE, "Connect: Failed authentication: %s", result.str());
      return false;
    }
    while (!cb->ctrl) {
      timedin = cb->cond.wait(timeout * 1000);
      if (!timedin) {
        logger.msg(VERBOSE, "Connect: Authentication timed out after %d ms",
                   timeout * 1000);
        return false;
      }
    }
    if (!cb->responseok) {
      logger.msg(VERBOSE, "Connect: Failed authentication: %s", cb->response);
      return false;
    }

    return true;

  } // end Connect

  bool FTPControl::SendCommand(const std::string& cmd, int timeout) {

    bool timedin;
    GlobusResult result;

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
      logger.msg(VERBOSE, "SendCommand: Failed: %s", cb->response);
      return false;
    }

    return true;

  } // end SendCommand

  bool FTPControl::SendCommand(const std::string& cmd, std::string& response,
                               int timeout) {

    bool timedin;
    GlobusResult result;

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
      logger.msg(VERBOSE, "SendCommand: Failed: %s", cb->response);
      return false;
    }

    response = cb->response;

    return true;

  } // end SendCommand

  bool FTPControl::SendData(const std::string& data,
                            const std::string& filename, int timeout) {

    bool timedin;
    GlobusResult result;

    if (!SendCommand("DCAU N", timeout)) {
      logger.msg(VERBOSE, "SendData: Failed sending DCAU command");
      return false;
    }

    if (!SendCommand("TYPE I", timeout)) {
      logger.msg(VERBOSE, "SendData: Failed sending TYPE command");
      return false;
    }

    std::string response;

    if (!SendCommand("PASV", response, timeout)) {
      logger.msg(VERBOSE, "SendData: Failed sending PASV command");
      return false;
    }

    std::string::size_type pos1 = response.find('(');
    if (pos1 == std::string::npos) {
      logger.msg(VERBOSE, "SendData: Server PASV response parsing failed: %s",
                 response);
      return false;
    }
    std::string::size_type pos2 = response.find(')', pos1 + 1);
    if (pos2 == std::string::npos) {
      logger.msg(VERBOSE, "SendData: Server PASV response parsing failed: %s",
                 response);
      return false;
    }

    globus_ftp_control_host_port_t passive_addr;
    passive_addr.port = 0;
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
    } else {
      logger.msg(VERBOSE, "SendData: Server PASV response parsing failed: %s",
                 response);
      return false;
    }

    result = globus_ftp_control_local_port(&control_handle, &passive_addr);
    if (!result) {
      logger.msg(VERBOSE, "SendData: Local port failed: %s", result.str());
      return false;
    }

    result = globus_ftp_control_local_type(&control_handle,
                                           GLOBUS_FTP_CONTROL_TYPE_IMAGE, 0);
    if (!result) {
      logger.msg(VERBOSE, "SendData: Local type failed: %s", result.str());
      return false;
    }

    cb->ctrl = false;
    cb->data = false;
    result = globus_ftp_control_send_command(&control_handle,
                                             ("STOR " + filename).c_str(),
                                             &ControlCallback, cb);
    if (!result) {
      logger.msg(VERBOSE, "SendData: Failed sending STOR command: %s",
                 result.str());
      return false;
    }

    result = globus_ftp_control_data_connect_write(&control_handle,
                                                   &DataConnectCallback, cb);
    if (!result) {
      logger.msg(VERBOSE, "SendData: Data connect write failed: %s",
                 result.str());
      return false;
    }
    while (!cb->data) {
      timedin = cb->cond.wait(timeout * 1000);
      if (!timedin) {
        logger.msg(VERBOSE, "SendData: Data connect write timed out after %d ms",
                   timeout * 1000);
        return false;
      }
    }
    while (!cb->ctrl) {
      timedin = cb->cond.wait(timeout * 1000);
      if (!timedin) {
        logger.msg(VERBOSE, "SendData: Data connect write timed out after %d ms",
                   timeout * 1000);
        return false;
      }
    }
    if (!cb->responseok) {
      logger.msg(VERBOSE, "SendData: Data connect write failed: %s",
                 cb->response);
      return false;
    }

    cb->data = false;
    cb->ctrl = false;
    result = globus_ftp_control_data_write(&control_handle,
                                           (globus_byte_t*)data.c_str(),
                                           data.size(), 0, GLOBUS_TRUE,
                                           &ReadWriteCallback, cb);
    if (!result) {
      logger.msg(VERBOSE, "SendData: Data write failed: %s", result.str());
      return false;
    }
    while (!cb->data) {
      timedin = cb->cond.wait(timeout * 1000);
      if (!timedin) {
        logger.msg(VERBOSE, "SendData: Data write timed out after %d ms",
                   timeout * 1000);
        return false;
      }
    }
    while (!cb->ctrl) {
      timedin = cb->cond.wait(timeout * 1000);
      if (!timedin) {
        logger.msg(VERBOSE, "SendData: Data write timed out after %d ms",
                   timeout * 1000);
        return false;
      }
    }
    if (!cb->responseok) {
      logger.msg(VERBOSE, "SendData: Data write failed: %s", cb->response);
      return false;
    }

    return true;

  } // end SendData

  bool FTPControl::Disconnect(int timeout) {

    bool timedin;
    GlobusResult result;
    bool res = true;

    if(!connected) return res;

    cb->ctrl = false;
    result = globus_ftp_control_quit(&control_handle, &ControlCallback, cb);
    if (!result) {
      logger.msg(VERBOSE, "Disconnect: Failed quitting: %s", result.str());
      return false;
    }
    while (!cb->ctrl) {
      timedin = cb->cond.wait(timeout * 1000);
      if (!timedin) {
        logger.msg(VERBOSE, "Disconnect: Quitting timed out after %d ms",
                   timeout * 1000);
        res = false;
      }
    }

    connected = false;
    cb->close = false;
    result = globus_ftp_control_force_close(&control_handle, &CloseCallback,
                                     cb->claim());
    if (!result) {
      cb->release();
      // Assuming only reason for failure here is that connection is 
      // already closed
      logger.msg(DEBUG, "Disconnect: Failed closing - ignoring: %s",
                 result.str());
    } else {
      // Need to wait for callback to make sure handle destruction will work
      // Hopefully forced close should never take long time
      while (!cb->close) {
        timedin = cb->cond.wait(timeout * 1000);
        if (!timedin) {
          logger.msg(ERROR, "Disconnect: Closing timed out after %d ms",
                     timeout * 1000);
          res = false;
        }
      }
    }

    result = globus_ftp_control_handle_destroy(&control_handle);
    if (!result) {
      // That means memory leak
      logger.msg(VERBOSE, "Disconnect: Failed to destroy handle: %s",
                 result.str());
      return false;
    }

    return true;

  } // end Disconnect

} // namespace Arc
