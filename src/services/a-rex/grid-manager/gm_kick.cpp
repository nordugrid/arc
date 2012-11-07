#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <arc/OptionParser.h>
#include <arc/StringConv.h>

#include "conf/GMConfig.h"
#include "jobs/commfifo.h"

int main(int argc,char* argv[]) {

  Arc::OptionParser options("[control_file]",
                            istring("gm-kick wakes up the A-REX corresponding to the given "
                                    "control file. If no file is given it uses the control directory "
                                    "found in the configuration file."));

  std::string conf_file;
  options.AddOption('c', "conffile",
                    istring("use specified configuration file"),
                    istring("file"), conf_file);

  std::list<std::string> params = options.Parse(argc, argv);

  std::string control_dir;
  if (params.empty()) {
    // Read from config
    ARex::GMConfig config(conf_file);
    if (!config.Load()) {
      std::cerr << "Could not load configuration from " << config.ConfigFile() << std::endl;
      return 1;
    }
    if (config.ControlDir().empty()) {
      std::cerr << "No control dir found in configuration file " << config.ConfigFile() << std::endl;
      return 1;
    }
    control_dir = config.ControlDir();
  }
  else {
    control_dir = params.front();
    if (control_dir[0] != '/') {
      char buf[1024];
      if (getcwd(buf, 1024) != NULL) control_dir = std::string(buf) + "/" + control_dir;
    }
    control_dir = control_dir.substr(0, control_dir.rfind('/'));
  }

  ARex::SignalFIFO(control_dir);
  return 0;
}

