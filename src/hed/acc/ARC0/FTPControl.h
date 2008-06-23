#ifndef __ARC_FTPCONTROL_H__
#define __ARC_FTPCONTROL_H__
#include <globus_ftp_control.h>
#include <arc/URL.h>

namespace Arc {

  class Logger;

  class FTPControl{

  public:
    FTPControl();
    ~FTPControl();

    bool connect(const URL &url, int timeout);
    bool sendCommand(std::string cmd, int timeout);
    bool disconnect(int timeout);

  private:
    static Logger logger;
    globus_ftp_control_handle_t control_handle;


  };

} // namespace Arc

#endif // __ARC_FTPCONTROL_H__



