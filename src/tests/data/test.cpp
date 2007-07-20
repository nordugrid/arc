#include "common/ArcConfig.h"
#include "common/Logger.h"
#include "common/URL.h"
#include "data/DMC.h"
#include "data/DataPoint.h"
#include "../../hed/libs/loader/Loader.h"

int main () {

  //  Arc::Logger::rootLogger.setThreshold(Arc::VERBOSE);
  Arc::Logger logger(Arc::Logger::rootLogger, "Test");
  Arc::LogStream logcerr(std::cerr);
  Arc::Logger::rootLogger.addDestination(logcerr);

  Arc::XMLNode client_doc("\
    <ArcConfig>\
     <ModuleManager>\
        <Path>../../libs/data/dmc/gridftp/.libs/</Path>\
        <Path>../../libs/data/dmc/rls/.libs/</Path>\
     </ModuleManager>\
     <Plugins><Name>dmcgridftp</Name></Plugins>\
     <Plugins><Name>dmcrls</Name></Plugins>\
     <DataManager name='gridftp' id='gridftp'></DataManager>\
     <DataManager name='rls' id='rls'></DataManager>\
    </ArcConfig>");

  Arc::Config client_config(client_doc);

  if(!client_config) {
    logger.msg(Arc::ERROR, "Failed to load client configuration");
    return -1;
  };
  Arc::Loader client_loader(&client_config);
  logger.msg(Arc::INFO, "DMCs are loaded");

  Arc::DataPoint* dp[3];
  dp[0] = Arc::DMC::GetDataPoint(Arc::URL("rls://hermione.hagrid.it.uu.se/test"));
  dp[1] = Arc::DMC::GetDataPoint(Arc::URL("gsiftp://harry.hagrid.it.uu.se:2811/vol2/dc2/test"));
  dp[2] = Arc::DMC::GetDataPoint(Arc::URL("ftp://download.nordugrid.org/test/README"));

  for (int k = 0; k < 3; k++) {
    std::list<Arc::DataPoint::FileInfo> files;
    dp[k]->list_files(files);
    for (std::list<Arc::DataPoint::FileInfo>::iterator i = files.begin();
	 i != files.end(); i++) {
      std::cout << "Name: " << i->name << std::endl;
      if(i->size != -1)
	std::cout << "Size: " << i->size << std::endl;
      if(i->created != -1)
	std::cout << "Created: " << i->created << std::endl;
      if(i->valid != -1)
	std::cout << "Valid: " << i->created << std::endl;
      if(!i->checksum.empty())
	std::cout << "Checksum: " << i->checksum << std::endl;
      for(std::list<Arc::URL>::iterator j = i->urls.begin();
	  j != i->urls.end(); j++)
	std::cout << "Location: " << *j << std::endl;
    }
    delete dp[k];
  }
}
