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

  Arc::DataPoint* dp1 =
    Arc::DMC::GetDataPoint(Arc::URL("gsiftp://harry.hagrid.it.uu.se:2811/vol2/dc2/test"));
  Arc::DataPoint* dp2 =
    Arc::DMC::GetDataPoint(Arc::URL("rls://hermione.hagrid.it.uu.se/test"));
  Arc::DataPoint* dp3 =
    Arc::DMC::GetDataPoint(Arc::URL("ftp://download.nordugrid.org/test/README"));
  std::cout << dp1->GetSize() << std::endl;
  std::cout << dp2->GetSize() << std::endl;
  std::cout << dp3->GetSize() << std::endl;
  delete dp1;
  delete dp2;
  delete dp3;
}
