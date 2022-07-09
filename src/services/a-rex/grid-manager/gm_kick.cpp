#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <arc/OptionParser.h>
#include <arc/StringConv.h>

#include "conf/GMConfig.h"
#include "jobs/CommFIFO.h"

int main(int argc,char* argv[]) {

  Arc::OptionParser options("[control_dir]",
                            istring("gm-kick wakes up the A-REX corresponding to the given "
                                    "control directory. If no directory is given it uses the control directory "
                                    "found in the configuration file."));

  std::string conf_file;
  options.AddOption('c', "conffile",
                    istring("use specified configuration file"),
                    istring("file"), conf_file);

  std::list<std::string> job_ids;
  options.AddOption('j', "jobid",
                    istring("inform about changes in particular job (can be used multiple times)"),
                    istring("id"), job_ids);

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
  }

  bool success = true;
  if(job_ids.empty()) {
    // general kick
    success = ARex::CommFIFO::Signal(control_dir);
  } else {
    for(std::list<std::string>::iterator id = job_ids.begin();
                        id != job_ids.end(); ++id) {
      if(!ARex::CommFIFO::Signal(control_dir,*id)) {
        success = false;
      };
    };
  };
  if(!success) {
    std::cerr<<"Failed reporting changes to A-REX"<<std::endl;
    return -1;
  };
  return 0;
}

