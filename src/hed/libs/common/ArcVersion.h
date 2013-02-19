// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_ARCVERSION_H__
#define __ARC_ARCVERSION_H__

/// \file ArcVersion.h
/** ARC API version */
#define ARC_VERSION "3.0.0"
/** ARC API version number */
#define ARC_VERSION_NUM 0x030000
/** ARC API major version number */
#define ARC_VERSION_MAJOR 3
/** ARC API minor version number */
#define ARC_VERSION_MINOR 0
/** ARC API patch number */
#define ARC_VERSION_PATCH 0

/// Arc namespace contains all core ARC classes.
namespace Arc {

  /// Determines ARC HED libraries version at runtime
  /**
   * ARC also provides pre-processor macros to determine the API version at
   * compile time in \ref ArcVersion.h.
   * \headerfile ArcVersion.h arc/ArcVersion.h
   */
  class ArcVersion {
  public:
    /// Major version number
    const unsigned int Major;
    /// Minor version number
    const unsigned int Minor;
    /// Patch version number
    const unsigned int Patch;
    /// Parses ver and fills major, minor and patch version values
    ArcVersion(const char* ver);
  };

  /// Use this object to obtain current ARC HED version 
  /// at runtime.
  extern const ArcVersion Version;

  // Front page for ARC SDK documentation
  /**
   * \mainpage
   * The ARC %Software Development Kit (SDK) is a set of tools that allow
   * manipulation of jobs and data in a Grid environment. The SDK is divided
   * into a set of <a href="modules.html">modules</a> which take care of
   * different aspects of Grid interaction.
   *
   * \section sec Quick Start
   * The following code is a minimal example showing how to submit a job to a
   * Grid resource using the ARC SDK:
   * \code
#include <arc/Logger.h>
#include <arc/UserConfig.h>
#include <arc/compute/Endpoint.h>
#include <arc/compute/Job.h>
#include <arc/compute/JobDescription.h>
#include <arc/compute/Submitter.h>

int main() {

  // Set up logging to stderr with level VERBOSE (a lot of output will be shown)
  Arc::LogStream logcerr(std::cerr);
  logcerr.setFormat(Arc::ShortFormat);
  Arc::Logger::getRootLogger().addDestination(logcerr);
  Arc::Logger::getRootLogger().setThreshold(Arc::VERBOSE);
  Arc::Logger logger(Arc::Logger::getRootLogger(), "jobsubmit");

  // UserConfig contains information on credentials and default services to use.
  // This form of the constructor is necessary to initialise the local job list.
  Arc::UserConfig usercfg("", "");

  // Simple job description which outputs hostname to stdout
  std::string jobdesc("&(executable=/bin/hostname)(stdout=stdout)");

  // Parse job description
  std::list<Arc::JobDescription> jobdescs;
  if (!Arc::JobDescription::Parse(jobdesc, jobdescs)) {
    logger.msg(Arc::ERROR, "Invalid job description");
    return 1;
  }

  // Use top-level NorduGrid information index to find resources
  Arc::Endpoint index("ldap://index1.nordugrid.org:2135/Mds-Vo-name=NorduGrid,o=grid",
                      Arc::Endpoint::REGISTRY,
                      "org.nordugrid.ldapegiis");
  std::list<Arc::Endpoint> services(1, index);

  // Do the submission
  std::list<Arc::Job> jobs;
  Arc::Submitter submitter(usercfg);
  if (submitter.BrokeredSubmit(services, jobdescs, jobs) != Arc::SubmissionStatus::NONE) {
    logger.msg(Arc::ERROR, "Failed to submit job");
    return 1;
  }

  // Write information on submitted job to local job list (~/.arc/jobs.xml)
  Arc::JobInformationStorageXML jobList(usercfg.JobListFile());
  if (!jobList.Write(jobs)) {
    logger.msg(Arc::WARNING, "Failed to write to local job list %s", usercfg.JobListFile());
  }

  // Job submitted ok
  std::cout << "Job submitted with job id " << jobs.front().JobID << std::endl;
  return 0;
}
   * \endcode
   * This code can be compiled with
   * \code
   * g++ -o submit -I/usr/include/libxml2 `pkg-config --cflags glibmm-2.4` -l arccompute submit.cpp
   * \endcode
   *
   * And this example shows how to copy a file to or from the Grid:
   * \code
#include <arc/Logger.h>
#include <arc/URL.h>
#include <arc/UserConfig.h>
#include <arc/data/DataHandle.h>
#include <arc/data/DataMover.h>

int main(int argc, char** argv) {

  // Set up logging to stderr with level VERBOSE (a lot of output will be shown)
  Arc::LogStream logcerr(std::cerr);
  logcerr.setFormat(Arc::ShortFormat);
  Arc::Logger::getRootLogger().addDestination(logcerr);
  Arc::Logger::getRootLogger().setThreshold(Arc::VERBOSE);
  Arc::Logger logger(Arc::Logger::getRootLogger(), "copy");

  if (argc != 3) {
    logger.msg(Arc::ERROR, "Usage: copy source destination");
    return 1;
  }

  // Set up source and destination objects
  Arc::UserConfig usercfg;
  Arc::URL src_url(argv[1]);
  Arc::URL dest_url(argv[2]);
  Arc::DataHandle src_handle(src_url, usercfg);
  Arc::DataHandle dest_handle(dest_url, usercfg);

  // Transfer should be insecure by default (most servers don't support encryption)
  // and passive if the client is behind a firewall
  Arc::DataMover mover;
  mover.secure(false);
  mover.passive(true);

  // If caching and URL mapping are not necessary default constructed objects can be used
  Arc::FileCache cache;
  Arc::URLMap map;

  // Call DataMover to do the transfer
  Arc::DataStatus result = mover.Transfer(*src_handle, *dest_handle, cache, map);

  if (!result.Passed()) {
    logger.msg(Arc::ERROR, "Copy failed: %s", std::string(result));
    return 1;
  }
  return 0;
}
   * \endcode
   * This example can be compiled with
   * \code
   * g++ -o copy -I/usr/include/libxml2 `pkg-config --cflags glibmm-2.4` -l arcdata copy.cpp
   * \endcode
   *
   * \version The version of the SDK that this documentation refers to can be
   * found from #ARC_VERSION.
   */


} // namespace Arc

#endif // __ARC_ARCVERSION_H__
