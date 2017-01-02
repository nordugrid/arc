// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <sys/stat.h>
#include <fstream>

#include <arc/OptionParser.h>
#include <arc/IString.h>
#include <arc/Logger.h>
#include <arc/compute/JobDescription.h>

#include "conf/GMConfig.h"
#include "conf/StagingConfig.h"
#include "jobs/JobDescriptionHandler.h"
#include "files/ControlFileContent.h"
#include "files/ControlFileHandling.h"


namespace ARex {
class GMJobMock : public GMJob {
public:
  GMJobMock(const JobId &job_id, const std::string &dir = "") : GMJob(job_id, Arc::User(), dir) {}
  ~GMJobMock() {}
  void SetLocalDescription(const Arc::JobDescription& desc) {
    if (local) {
      delete local;
    }
    local = new JobLocalDescription;
    *local = desc;
    local->globalid = job_id;
  }
  std::list<FileData>& GetOutputdata() { return local->outputdata; }
};
}

int main(int argc, char **argv) {

  Arc::Logger logger(Arc::Logger::getRootLogger(), "test_write_grami_file");
  Arc::LogStream logcerr(std::cerr);
  Arc::Logger::getRootLogger().addDestination(logcerr);
  Arc::Logger::getRootLogger().setThreshold(Arc::WARNING);

  Arc::OptionParser options(istring("[job description input]"),
                            istring("Tool for writing the grami file representation of a job description file."));

  std::string gramiFileName;
  options.AddOption('g', "grami",
                    istring("Name of grami file"),
                    istring("filename"), gramiFileName);

  std::string confFileName;
  options.AddOption('z', "conf",
                    istring("Configuration file to load"),
                    istring("arc.conf"), confFileName);

  std::string sessionDir = "";
  options.AddOption('s', "session-dir",
                    istring("Session directory to use"),
                    istring("directory"), sessionDir);

  std::string debug;
  options.AddOption('d', "debug",
                    istring("FATAL, ERROR, WARNING, INFO, VERBOSE or DEBUG"),
                    istring("debuglevel"), debug);

  std::list<std::string> descriptions = options.Parse(argc, argv);
  if (descriptions.empty()) {
    std::cout << Arc::IString("Use --help option for detailed usage information") << std::endl;
    return 1;
  }

  if (!debug.empty()) Arc::Logger::getRootLogger().setThreshold(Arc::string_to_level(debug));

  if (descriptions.empty()) {
    logger.msg(Arc::ERROR, "No job description file name provided.");
    return 1;
  }

  std::list<Arc::JobDescription> jobdescs;
  if (!Arc::JobDescription::Parse(descriptions.front(), jobdescs) || jobdescs.empty()) {
    logger.msg(Arc::ERROR, "Unable to parse job description input: %s", descriptions.front());
    return 1;
  }
  
  ARex::GMConfig gmc(confFileName);
  if (!gmc.Load()) {
    logger.msg(Arc::ERROR, "Unable to load ARC configuration file.");
    return 1;
  }
  gmc.SetShareID(Arc::User());
  
  ARex::StagingConfig sconf(gmc);

  if (sessionDir.empty() && !gmc.SessionRoots().empty()) {
    sessionDir = gmc.SessionRoots().front();
  }
  
  ARex::GMJobMock gmjob(gramiFileName, sessionDir + "/" + gramiFileName);
  gmjob.SetLocalDescription(jobdescs.front());

  ARex::JobDescriptionHandler jdh(gmc);
  
  if (!jdh.write_grami(jobdescs.front(), gmjob, NULL)) {
    logger.msg(Arc::ERROR, "Unable to write grami file: %s", "job." + gmjob.get_id() + ".grami");
    return 1;
  }
  
  if (!job_output_write_file(gmjob, gmc, gmjob.GetOutputdata())) {
    logger.msg(Arc::ERROR, "Unable to write 'output' file: %s", "job." + gmjob.get_id() + ".output");
    return 1;
  }

  std::cout << "grami file written to " << ("job." + gmjob.get_id() + ".grami") << std::endl;

  return 0;
}
