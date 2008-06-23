#include "FTPControl.h"
#include "GlobusErrorUtils.h"

#include <arc/ArcConfig.h>
#include <arc/data/DataBufferPar.h>
#include <arc/data/DataHandle.h>
#include <arc/StringConv.h>
#include <arc/Logger.h>
#include <arc/Thread.h>

#include <map>
#include <iostream>
#include <algorithm>

struct cbarg {
  Arc::SimpleCondition cond;
  std::string response;
  bool data;
  bool ctrl;
};

static void ControlCallback(void *arg,
			    globus_ftp_control_handle_t*,
			    globus_object_t*,
			    globus_ftp_control_response_t *response) {
  cbarg *cb = (cbarg *)arg;
  if (response && response->response_buffer)
    cb->response.assign((const char *)response->response_buffer,
			response->response_length);
  else
    cb->response.clear();
  cb->ctrl = true;
  cb->cond.signal();
}

namespace Arc{

  Logger FTPControl::logger(Logger::getRootLogger(), "FTPControl");
  
  FTPControl::FTPControl(){}
  
  FTPControl::~FTPControl(){}

  bool FTPControl::connect(const URL &url, int timeout){
    
    cbarg cb;
    
    GlobusResult result = globus_ftp_control_handle_init(&control_handle);
    if(!result){
      logger.msg(ERROR, "globus_ftp_control_handle_init: %s", result.str());
      return false;
    }
    
    cb.ctrl = false;
    result = globus_ftp_control_connect(&control_handle,
					const_cast<char *>(url.Host().c_str()),
					url.Port(), &ControlCallback, &cb);
    if(!result){
      logger.msg(ERROR, "globus_ftp_control_control_connect: %s", result.str());
      return false;
    }
   
    bool timedin;
    while (!cb.ctrl)
      timedin = cb.cond.wait(timeout*1000);
    if(!timedin){
      logger.msg(ERROR, "globus_ftp_control_control_connect timed out after %d milliseconds", timeout*1000);      
      return false;
    }

    // should do some clever thing here to integrate ARC1 security framework
    globus_ftp_control_auth_info_t auth;
    result = globus_ftp_control_auth_info_init(&auth, GSS_C_NO_CREDENTIAL, GLOBUS_TRUE,
					       const_cast<char*>("ftp"),
					       const_cast<char*>("user@"),
					       GLOBUS_NULL, GLOBUS_NULL);
    if(!result){
      logger.msg(ERROR, "globus_ftp_control_auth_info_init: %s", result.str());
      return false;
    }
    
    
    cb.ctrl = false;

    result= globus_ftp_control_authenticate(&control_handle, &auth, GLOBUS_TRUE,
				    &ControlCallback, &cb);
    if(!result){
      logger.msg(ERROR, "globus_ftp_control_authenticate: %s", result.str());
      return false;
    }

    while (!cb.ctrl)
      timedin = cb.cond.wait(timeout*1000);
    if(!timedin){
      logger.msg(ERROR, "globus_ftp_control_authenticate timed out after %d milliseconds", timeout*1000);      
      return false;
    }

    return true;

  } //end connect
  
  bool FTPControl::sendCommand(std::string cmd, int timeout){
    
    cbarg cb;
    
    cb.ctrl = false;
    GlobusResult result = globus_ftp_control_send_command(&control_handle, cmd.c_str(),
							 &ControlCallback, &cb);
    if(!result){
      logger.msg(ERROR, "globus_ftp_control_send_command: %s", result.str());      
      return false;
    }
    
    bool timedin;
    while (!cb.ctrl)
      timedin = cb.cond.wait(timeout*1000);
    if(!timedin){
      logger.msg(ERROR, "globus_ftp_control_send_command timed out after %d milliseconds", timeout*1000);            
      return false;
    }
  
  }//end sendCommand

  bool FTPControl::disconnect(int timeout){
    
    cbarg cb;

    cb.ctrl = false;    
    GlobusResult result = globus_ftp_control_quit(&control_handle, &ControlCallback, &cb);
    if(!result){
      logger.msg(ERROR, "globus_ftp_control_quit: %s", result.str());      
      return false;
    }
    
    bool timedin;
    while (!cb.ctrl)
      timedin = cb.cond.wait(timeout*1000);
    if(!timedin){
      logger.msg(ERROR, "globus_ftp_control_control_quit timed out after %d milliseconds", timeout*1000);            
      return false;
    }

    result = globus_ftp_control_handle_destroy(&control_handle);
    if(!result){
      logger.msg(ERROR, "globus_ftp_handle_destroy: %s", result.str());      
      return false;
    }
    
  }//end disconnect

} //end namespace
