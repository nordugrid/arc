// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_FTPCONTROL_H__
#define __ARC_FTPCONTROL_H__

#include <globus_ftp_control.h>

#include <arc/URL.h>

namespace Arc {

  class Logger;

  class FTPControl {

  public:
    FTPControl();
    ~FTPControl();

    bool Connect(const URL& url, const std::string& proxyPath,
                 const std::string& certificatePath,
                 const std::string& keyPath, int timeout);
    bool SendCommand(const std::string& cmd, int timeout);
    bool SendCommand(const std::string& cmd, std::string& response,
                     int timeout);
    bool SendData(const std::string& data, const std::string& filename,
                  int timeout);
    bool Disconnect(int timeout);

  private:
    static Logger logger;
    globus_ftp_control_handle_t control_handle;
  };

} // namespace Arc

#endif // __ARC_FTPCONTROL_H__
