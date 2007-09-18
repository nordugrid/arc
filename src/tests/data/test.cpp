#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/ArcConfig.h>
#include <arc/Logger.h>
#include <arc/URL.h>
#include <arc/data/DMC.h>
#include <arc/data/DataHandle.h>
#include <arc/loader/Loader.h>

int main () {

  //  Arc::Logger::rootLogger.setThreshold(Arc::VERBOSE);
  Arc::Logger logger(Arc::Logger::rootLogger, "Test");
  Arc::LogStream logcerr(std::cerr);
  Arc::Logger::rootLogger.addDestination(logcerr);

  Arc::XMLNode client_doc("\
    <ArcConfig>\
     <ModuleManager>\
        <Path>../../hed/dmc/gridftp/.libs/</Path>\
        <Path>../../hed/dmc/http/.libs/</Path>\
        <Path>../../hed/dmc/lfc/.libs/</Path>\
        <Path>../../hed/dmc/rls/.libs/</Path>\
     </ModuleManager>\
     <Plugins><Name>dmcgridftp</Name></Plugins>\
     <Plugins><Name>dmchttp</Name></Plugins>\
     <Plugins><Name>dmclfc</Name></Plugins>\
     <Plugins><Name>dmcrls</Name></Plugins>\
     <DataManager name='gridftp' id='gridftp'></DataManager>\
     <DataManager name='http' id='http'></DataManager>\
     <DataManager name='rls' id='rls'></DataManager>\
     <DataManager name='lfc' id='lfc'></DataManager>\
    </ArcConfig>");

  Arc::Config client_config(client_doc);

  if(!client_config) {
    logger.msg(Arc::ERROR, "Failed to load client configuration");
    return -1;
  }
  Arc::Loader client_loader(&client_config);
  logger.msg(Arc::INFO, "DMCs are loaded");

  Arc::DataHandle dp[5] = {
    Arc::URL("rls://hermione.hagrid.it.uu.se/test"),
    Arc::URL("gsiftp://harry.hagrid.it.uu.se:2811/vol2/dc2/test"),
    Arc::URL("ftp://download.nordugrid.org/test/README"),
    Arc::URL("http://www.grid.tsl.uu.se/"),
    Arc::URL("lfc://lfc-atlas.in2p3.fr//grid/atlas/dq2/trig1_misal1_mc12/trig1_misal1_mc12.008248.AlpgenJimmyWtaunuNp2_pt20_filt3jet.recon.AOD.v12000601_tid006559/trig1_misal1_mc12.008248.AlpgenJimmyWtaunuNp2_pt20_filt3jet.recon.AOD.v12000601_tid006559._00024.pool.root.3")
  };

  for (int k = 0; k < 5; k++) {
    std::list<Arc::DataPoint::FileInfo> files;
    if(!dp[k]) continue;
    dp[k]->list_files(files);
    for (std::list<Arc::DataPoint::FileInfo>::iterator i = files.begin();
	 i != files.end(); i++) {
      std::cout << "Name: " << i->name << std::endl;
      if(i->size != (unsigned long long int)(-1))
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
  }
}
