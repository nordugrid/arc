#include <arc/Logger.h>
#include <arc/OptionParser.h>
#include <arc/ArcLocation.h>
#include <arc/User.h>
#include <arc/data/FileCache.h>

#include "DTR.h"

/**
 * Executable used for post-transfer cache processing, ie linking or
 * copying cache files to their destination. It is a separate executable
 * so that these operations can run under a different uid from the main
 * ARC server process, and so that it is independent from other ARC threads
 * (avoiding the problems caused by fork() in a multi-threaded environment).
 */
int main(int argc, char** argv) {

  setlocale(LC_ALL, "");

  // log to stderr
  Arc::LogStream logcerr(std::cerr);
  Arc::Logger::getRootLogger().addDestination(logcerr);
  Arc::Logger::getRootLogger().setThreshold(Arc::WARNING);
  Arc::Logger logger(Arc::Logger::getRootLogger(), "CacheManager");

  Arc::ArcLocation::Init(argv[0]);

  Arc::OptionParser options(istring("job_id url destination"),
                            istring("CacheManager copies or links the file "
                                    "url which exists in the cache to "
                                    "destination"));

  bool link = false;
  bool copy = false;
  options.AddOption('l', "link", istring("link the cache file"), link);
  options.AddOption('c', "copy", istring("copy the cache file"), copy);

  bool executable = false;
  options.AddOption('e', "executable", istring("file is executable"), executable);

  std::string conf_file;
  options.AddOption('f', "conf", istring("configuration file"), "path", conf_file);

  std::string debug;
  options.AddOption('d', "debug",
                    istring("FATAL, ERROR, WARNING, INFO, VERBOSE or DEBUG"),
                    istring("debuglevel"), debug);

  int uid = 0;
  int gid = 0;
  options.AddOption('u', "uid", istring("uid of destination owner"), "uid", uid);
  options.AddOption('g', "gid", istring("gid of destination owner"), "gid", gid);

  std::list<std::string> params = options.Parse(argc, argv);

  // If debug is specified as argument, it should be set before loading the configuration.
  if (!debug.empty())
    Arc::Logger::getRootLogger().setThreshold(Arc::string_to_level(debug));

  if (params.size() != 3) {
    logger.msg(Arc::ERROR, "Wrong number of parameters specified");
    return 1;
  }

  // one of link or copy must be set
  if ((!link) && (!copy)) {
    logger.msg(Arc::ERROR, "One of -l and -c must be specified");
    return 1;
  }

  if (conf_file.empty()) {
    logger.msg(Arc::ERROR, "No configuration specified");
    return 1;
  }

  std::list<std::string>::iterator it = params.begin();
  std::string job_id = *it;
  ++it;
  std::string source_url = *it;
  ++it;
  std::string destination = *it;

  std::ifstream cache_conf(conf_file.c_str());
  if(!cache_conf) {
    logger.msg(Arc::ERROR, "Failed to open configuration file: %s", conf_file);
    return 1;
  }

  DataStaging::CacheParameters cache_params;
  cache_conf>>cache_params;
  cache_conf.close();

  Arc::FileCache cache(cache_params.cache_dirs,
                       cache_params.remote_cache_dirs,
                       cache_params.drain_cache_dirs,
                       job_id,
                       uid,
                       gid);
  if (!cache) {
    logger.msg(Arc::ERROR, "Error creating cache");
    return 1;
  }

  if (!cache.Link(destination, source_url, copy, executable, true)) {
    logger.msg(Arc::ERROR, "Error linking/copying cache file");
    return 1;
  }

  return 0;
}
