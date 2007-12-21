#include <fstream>
#include <string>
#include <arc/Logger.h>

namespace Arc {

class ClientTool {
 private:
  bool success_;
  Arc::LogStream logcerr_;
  std::ofstream logfile_;
  Arc::LogStream logdest_;
  std::string name_;
  int option_idx_;
 public:
  ClientTool(int argc,char* argv[],const std::string& name,const std::string& optstr = "");
  virtual ~ClientTool(void);
  virtual bool ProcessOption(char option,char* option_arg);
  virtual void PrintHelp(void);
  operator bool(void) { return success_; };
  int FirstOption(void) { return option_idx_; };
};

}

