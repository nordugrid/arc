#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>
#include <string>
#include <fstream>

#include <cppunit/extensions/HelperMacros.h>

#include <arc/ArcConfig.h>
#include <arc/message/SOAPEnvelope.h>
#include <arc/message/PayloadRaw.h>
#include <arc/infosys//RegisteredService.h>


class RegisteredServiceTest
  : public CppUnit::TestFixture {

  CPPUNIT_TEST_SUITE(RegisteredServiceTest);
  CPPUNIT_TEST(TestRegisteredService);
  CPPUNIT_TEST_SUITE_END();

public:
  void setUp();
  void tearDown();
  void TestRegisteredService();
};

static const char* xml_str = "\
<?xml version=\"1.0\" encoding=\"UTF-8\"?>\
<SOAP-ENV:Envelope \
xmlns:SOAP-ENV=\"http://schemas.xmlsoap.org/soap/envelope/\" \
xmlns:SOAP-ENC=\"http://schemas.xmlsoap.org/soap/encoding/\" \
xmlns:a-rex=\"http://www.nordugrid.org/schemas/a-rex\" \
xmlns:bes-factory=\"http://schemas.ggf.org/bes/2006/08/bes-factory\" \
xmlns:jsdl=\"http://schemas.ggf.org/jsdl/2005/11/jsdl\" \
xmlns:wsa=\"http://www.w3.org/2005/08/addressing\" \
xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\" \
xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"><SOAP-ENV:Body><bes-factory:CreateActivity><bes-factory:ActivityDocument><JobDefinition xmlns=\"http://schemas.ggf.org/jsdl/2005/11/jsdl\"> \
    <JobDescription> \
        <JobIdentification> \
            <JobName>Demo</JobName> \
            <JobProject>HPCPInterop</JobProject> \
        </JobIdentification> \
        <Application> \
            <HPCProfileApplication xmlns=\"http://schemas.ggf.org/jsdl/2006/07/jsdl-hpcpa\"> \
                <Executable>/home/interop/demo_job.sh</Executable> \
                <Argument>60</Argument> \
                <Output>demo_job.out.%J</Output> \
                <WorkingDirectory>/home/interop/username</WorkingDirectory> \
            </HPCProfileApplication> \
        </Application> \
        <Resources> \
            <TotalCPUCount> \
                <Exact>1</Exact> \
            </TotalCPUCount> \
        </Resources> \
    </JobDescription> \
</JobDefinition></bes-factory:ActivityDocument></bes-factory:CreateActivity></SOAP-ENV:Body></SOAP-ENV:Envelope>";

  class MyRegisteredService: public Arc::RegisteredService
  {
      public:
          /** Example contructor - Server takes at least it's configuration subtree */
          MyRegisteredService(Arc::Config* cfg):RegisteredService(cfg) 
                      { std::cout << "My Service called!" << std::endl;
                        // VERBOSE                    
                        std::string config_str;
                        cfg->GetXML(config_str, true);
                        logger.msg(Arc::VERBOSE, "The config: \n%s \n", config_str);
                      };

          virtual ~MyRegisteredService(void) { };
          virtual Arc::MCC_Status process(Arc::Message&,Arc::Message&) {return Arc::MCC_Status(Arc::STATUS_OK);};
  };


void RegisteredServiceTest::setUp() {
}


void RegisteredServiceTest::tearDown() {
}


void RegisteredServiceTest::TestRegisteredService() {
  Arc::Logger logger(Arc::Logger::rootLogger, "RegisteredService-Test");
  Arc::LogStream logcerr(std::cerr);
  Arc::Logger::rootLogger.addDestination(logcerr);

  Arc::SOAPEnvelope soap(xml_str);


  Arc::MessagePayload *m = new Arc::PayloadRaw();

  std::string cfg_str="";
  cfg_str +="\n";
  cfg_str +="    <ArcConfig\n";
  cfg_str +="      xmlns=\"http://www.nordugrid.org/schemas/ArcConfig/2007\"\n";
  cfg_str +="      xmlns:tcp=\"http://www.nordugrid.org/schemas/ArcMCCTCP/2007\">\n";
  cfg_str +="     <ModuleManager>\n";
  cfg_str +="        <Path>.libs/</Path>\n";
  cfg_str +="        <Path>../../hed/mcc/http/.libs/</Path>\n";
  cfg_str +="        <Path>../../hed/mcc/soap/.libs/</Path>\n";
  cfg_str +="        <Path>../../hed/mcc/tls/.libs/</Path>\n";
  cfg_str +="        <Path>../../hed/mcc/tcp/.libs/</Path>\n";
  cfg_str +="     </ModuleManager>\n";
  cfg_str +="     <Plugins><Name>mcctcp</Name></Plugins>\n";
  cfg_str +="     <Plugins><Name>mcctls</Name></Plugins>\n";
  cfg_str +="     <Plugins><Name>mcchttp</Name></Plugins>\n";
  cfg_str +="     <Plugins><Name>mccsoap</Name></Plugins>\n";
  cfg_str +="     <Chain>\n";
  cfg_str +="      <Component name='tcp.client' id='tcp'><tcp:Connect><tcp:Host>example.org</tcp:Host><tcp:Port>50000</tcp:Port></tcp:Connect></Component>\n";
  cfg_str +="      <Component name='http.client' id='http'><next id='tcp'/><Method>POST</Method><Endpoint>echo</Endpoint></Component>\n";
  cfg_str +="      <Component name='soap.client' id='soap' entry='soap'><next id='http'/></Component>\n";
  cfg_str +="     </Chain>\n";
  cfg_str +="    </ArcConfig>";
  
  Arc::Config cfg(cfg_str);
  MyRegisteredService myservice(&cfg);

  CPPUNIT_ASSERT(true);
}


CPPUNIT_TEST_SUITE_REGISTRATION(RegisteredServiceTest);

