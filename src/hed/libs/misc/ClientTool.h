#include <fstream>
#include <string>
#include <arc/Logger.h>

namespace Arc {

class ClientTool {
 protected:
  bool success_;
  Arc::LogStream logcerr_;
  std::ofstream logfile_;
  Arc::LogStream logdest_;
  std::string name_;
  int option_idx_;
  bool ProcessOptions(int argc,char* argv[],const std::string& optstr = "");
 public:
  ClientTool(const std::string& name);
  virtual ~ClientTool(void);
  virtual bool ProcessOption(char option,char* option_arg);
  virtual void PrintHelp(void);
  int FirstOption(void) { return option_idx_; };
  operator bool(void) { return success_; };
};

}

