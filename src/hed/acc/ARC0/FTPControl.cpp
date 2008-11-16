#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/Logger.h>
#include <arc/Thread.h>
#include <arc/globusutils/GlobusErrorUtils.h>
#include <arc/globusutils/GSSCredential.h>

#include "FTPControl.h"

struct cbarg {
  Arc::SimpleCondition cond;
  std::string response;
  bool responseok;
  bool data;
  bool ctrl;
};

static void ControlCallback(void *arg, globus_ftp_control_handle_t*,
			    globus_object_t *error,
			    globus_ftp_control_response_t *response) {
  cbarg *cb = (cbarg*)arg;
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
			    unsigned int, globus_bool_t, globus_object_t*) {
  cbarg *cb = (cbarg*)arg;
  cb->data = true;
  cb->cond.signal();
}

static void ReadWriteCallback(void *arg, globus_ftp_control_handle_t*,
			      globus_object_t*, globus_byte_t*, globus_size_t,
			      globus_off_t, globus_bool_t) {
  cbarg *cb = (cbarg*)arg;
  cb->data = true;
  cb->cond.signal();
}

static int pwck(char*, int, int) {
  return -1;
}

namespace Arc {

  Logger FTPControl::logger(Logger::getRootLogger(), "FTPControl");

  FTPControl::FTPControl() {
    globus_module_activate(GLOBUS_FTP_CONTROL_MODULE);
  }

  FTPControl::~FTPControl() {
    globus_module_deactivate(GLOBUS_FTP_CONTROL_MODULE);
  }

  bool FTPControl::Connect(const URL& url,
			   const std::string& proxyPath,
			   const std::string& certificatePath,
			   const std::string& keyPath,
			   int timeout) {

    cbarg cb;
    bool timedin;
    GlobusResult result;

    result = globus_ftp_control_handle_init(&control_handle);
    if (!result) {
      logger.msg(ERROR, "Connect: Failed to init handle: %s", result.str());
      return false;
    }

    cb.ctrl = false;
    result = globus_ftp_control_connect(&control_handle,
					const_cast<char*>(url.Host().c_str()),
					url.Port(), &ControlCallback, &cb);
    if (!result) {
      logger.msg(ERROR, "Connect: Failed to connect: %s", result.str());
      return false;
    }
    while (!cb.ctrl) {
      timedin = cb.cond.wait(timeout * 1000);
      if (!timedin) {
	logger.msg(ERROR, "Connect: Connecting timed out after %d ms",
		   timeout * 1000);
	return false;
      }
    }
    if (!cb.responseok) {
      logger.msg(ERROR, "Connect: Failed to connect: %s", cb.response);
      return false;
    }

    GSSCredential handle(proxyPath, certificatePath, keyPath);

    globus_ftp_control_auth_info_t auth;
    result = globus_ftp_control_auth_info_init(&auth, handle, GLOBUS_TRUE,
					       const_cast<char*>("ftp"),
					       const_cast<char*>("user@"),
					       GLOBUS_NULL, GLOBUS_NULL);
    if (!result) {
      logger.msg(ERROR, "Connect: Failed to init auth info handle: %s",
		 result.str());
      return false;
    }

    cb.ctrl = false;
    result = globus_ftp_control_authenticate(&control_handle, &auth,
					     GLOBUS_TRUE,
					     &ControlCallback, &cb);
    if (!result) {
      logger.msg(ERROR, "Connect: Failed authentication: %s", result.str());
      return false;
    }
    while (!cb.ctrl) {
      timedin = cb.cond.wait(timeout * 1000);
      if (!timedin) {
	logger.msg(ERROR, "Connect: Authentication timed out after %d ms",
		   timeout * 1000);
	return false;
      }
    }
    if (!cb.responseok) {
      logger.msg(ERROR, "Connect: Failed authentication: %s", cb.response);
      return false;
    }

    return true;

  } // end Connect

  bool FTPControl::SendCommand(const std::string& cmd, int timeout) {

    cbarg cb;
    bool timedin;
    GlobusResult result;

    cb.ctrl = false;
    result = globus_ftp_control_send_command(&control_handle, cmd.c_str(),
					     &ControlCallback, &cb);
    if (!result) {
      logger.msg(ERROR, "SendCommand: Failed: %s", result.str());
      return false;
    }
    while (!cb.ctrl) {
      timedin = cb.cond.wait(timeout * 1000);
      if (!timedin) {
	logger.msg(ERROR, "SendCommand: Timed out after %d ms", timeout * 1000);
	return false;
      }
    }
    if (!cb.responseok) {
      logger.msg(ERROR, "SendCommand: Failed: %s", cb.response);
      return false;
    }

    return true;

  } // end SendCommand

  bool FTPControl::SendCommand(const std::string& cmd, std::string& response,
			       int timeout) {

    cbarg cb;
    bool timedin;
    GlobusResult result;

    cb.ctrl = false;
    result = globus_ftp_control_send_command(&control_handle, cmd.c_str(),
					     &ControlCallback, &cb);
    if (!result) {
      logger.msg(ERROR, "SendCommand: Failed: %s", result.str());
      return false;
    }
    while (!cb.ctrl) {
      timedin = cb.cond.wait(timeout * 1000);
      if (!timedin) {
	logger.msg(ERROR, "SendCommand: Timed out after %d ms", timeout * 1000);
	return false;
      }
    }
    if (!cb.responseok) {
      logger.msg(ERROR, "SendCommand: Failed: %s", cb.response);
      return false;
    }

    response = cb.response;

    return true;

  } // end SendCommand

  bool FTPControl::SendData(const std::string& data,
			    const std::string& filename, int timeout) {

    cbarg cb;
    bool timedin;
    GlobusResult result;

    if (!SendCommand("DCAU N", timeout)) {
      logger.msg(ERROR, "SendData: Failed sending DCAU command");
      return false;
    }

    if (!SendCommand("TYPE I", timeout)) {
      logger.msg(ERROR, "SendData: Failed sending TYPE command");
      return false;
    }

    std::string response;

    if (!SendCommand("PASV", response, timeout)) {
      logger.msg(ERROR, "SendData: Failed sending PASV command");
      return false;
    }

    std::string::size_type pos1 = response.find('(');
    std::string::size_type pos2 = response.find(')', pos1 + 1);

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
	       &port_low) == 6)
      passive_addr.port = 256 * port_high + port_low;

    result = globus_ftp_control_local_port(&control_handle, &passive_addr);
    if (!result) {
      logger.msg(ERROR, "SendData: Local port failed: %s", result.str());
      return false;
    }

    result = globus_ftp_control_local_type(&control_handle,
					   GLOBUS_FTP_CONTROL_TYPE_IMAGE, 0);
    if (!result) {
      logger.msg(ERROR, "SendData: Local type failed: %s", result.str());
      return false;
    }

    result = globus_ftp_control_send_command(&control_handle,
					     ("STOR " + filename).c_str(),
					     &ControlCallback, &cb);
    if (!result) {
      logger.msg(ERROR, "SendData: Failed sending STOR command: %s",
		 result.str());
      return false;
    }

    cb.data = false;
    cb.ctrl = false;
    result = globus_ftp_control_data_connect_write(&control_handle,
						   &ConnectCallback, &cb);
    if (!result) {
      logger.msg(ERROR, "SendData: Data connect write failed: %s",
		 result.str());
      return false;
    }
    while (!cb.data) {
      timedin = cb.cond.wait(timeout * 1000);
      if (!timedin) {
	logger.msg(ERROR, "SendData: Data connect write timed out after %d ms",
		   timeout * 1000);
	return false;
      }
    }
    while (!cb.ctrl) {
      timedin = cb.cond.wait(timeout * 1000);
      if (!timedin) {
	logger.msg(ERROR, "SendData: Data connect write timed out after %d ms",
		   timeout * 1000);
	return false;
      }
    }
    if (!cb.responseok) {
      logger.msg(ERROR, "SendData: Data connect write failed: %s", cb.response);
      return false;
    }

    cb.data = false;
    cb.ctrl = false;
    result = globus_ftp_control_data_write(&control_handle,
					   (globus_byte_t*)data.c_str(),
					   data.size(), 0, GLOBUS_TRUE,
					   &ReadWriteCallback, &cb);
    if (!result) {
      logger.msg(ERROR, "SendData: Data write failed: %s", result.str());
      return false;
    }
    while (!cb.data) {
      timedin = cb.cond.wait(timeout * 1000);
      if (!timedin) {
	logger.msg(ERROR, "SendData: Data write timed out after %d ms",
		   timeout * 1000);
	return false;
      }
    }
    while (!cb.ctrl) {
      timedin = cb.cond.wait(timeout * 1000);
      if (!timedin) {
	logger.msg(ERROR, "SendData: Data write timed out after %d ms",
		   timeout * 1000);
	return false;
      }
    }
    if (!cb.responseok) {
      logger.msg(ERROR, "SendData: Data write failed: %s", cb.response);
      return false;
    }

    return true;

  } // end SendData

  bool FTPControl::Disconnect(int timeout) {

    cbarg cb;
    bool timedin;
    GlobusResult result;

    cb.ctrl = false;
    result = globus_ftp_control_quit(&control_handle, &ControlCallback, &cb);
    if (!result) {
      logger.msg(ERROR, "Disconnect: Failed quitting: %s", result.str());
      return false;
    }
    while (!cb.ctrl) {
      timedin = cb.cond.wait(timeout * 1000);
      if (!timedin) {
	logger.msg(ERROR, "Disconnect: Quitting timed out after %d ms",
		   timeout * 1000);
	return false;
      }
    }

    result = globus_ftp_control_handle_destroy(&control_handle);
    if (!result) {
      logger.msg(ERROR, "Disconnect: Failed to destroy handle: %s",
		 result.str());
      return false;
    }

    return true;

  } // end Disconnect

} // namespace Arc
