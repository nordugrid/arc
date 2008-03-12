#include <iostream>

#include <signal.h>

#include <arc/XMLNode.h>
#include <arc/ArcConfig.h>
#include <arc/loader/Loader.h>
#include <arc/data/URLMap.h>
#include <arc/data/DataMover.h>
#include "DataStatus.h"

int main(int argc,char* argv[]) {
  signal(SIGTTOU,SIG_IGN);
  if(argc < 2) {
    std::cerr<<"Missing source URL"<<std::endl;
    return -1;
  };
  if(argc < 3) {
    std::cerr<<"Missing destination URL"<<std::endl;
    return -1;
  };
  Arc::LogStream logcerr(std::cerr, "DataTest");
  Arc::Logger::getRootLogger().addDestination(logcerr);
  Arc::Logger::getRootLogger().setThreshold(Arc::VERBOSE);
  const char* config_str = "<?xml version=\"1.0\"?>\
    <ArcConfig xmlns=\"http://www.nordugrid.org/schemas/ArcConfig/2007\">\
      <ModuleManager>\
        <Path>../../dmc/file/.libs/</Path>\
        <Path>../../dmc/gridftp/.libs/</Path>\
        <Path>../../dmc/http/.libs/</Path>\
        <Path>../../dmc/rls/.libs/</Path>\
        <Path>../../dmc/lfc/.libs/</Path>\
      </ModuleManager>\
      <Plugins><Name>dmcfile</Name></Plugins>\
      <DataManager name=\"file\" id=\"file\"/>\
      <Plugins><Name>dmcgridftp</Name></Plugins>\
      <DataManager name=\"gridftp\" id=\"gridftp\"/>\
      <Plugins><Name>dmchttp</Name></Plugins>\
      <DataManager name=\"http\" id=\"http\"/>\
      <Plugins><Name>dmcrls</Name></Plugins>\
      <DataManager name=\"rls\" id=\"rls\"/>\
      <Plugins><Name>dmclfc</Name></Plugins>\
      <DataManager name=\"lfc\" id=\"lfc\"/>\
    </ArcConfig>";
  Arc::XMLNode config_xml(config_str);
  Arc::Config config(config_xml);
  Arc::Loader loader(&config);

  Arc::DataMover mover;
  mover.retry(false);
  std::string failure_description;
  Arc::DataPoint* source = Arc::DMC::GetDataPoint(Arc::URL(argv[1]));
  Arc::DataPoint* dest = Arc::DMC::GetDataPoint(Arc::URL(argv[2]));
  Arc::URLMap urlmap;
  Arc::DataCache datacache;
  Arc::DataStatus res = mover.Transfer(*source,*dest,datacache,urlmap,failure_description);
  if(!res) {
    std::cerr<<"Transfer failed: " << res << " - " << failure_description << std::endl;
  };
  return 0;
}

