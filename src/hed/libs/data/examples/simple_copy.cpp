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
