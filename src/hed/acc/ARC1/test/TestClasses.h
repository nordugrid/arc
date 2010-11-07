#ifndef __TEST_CLASSES__
#define __TEST_CLASSES__

namespace Arc {

  class Test_ClientSOAP {
    public:
      Test_ClientSOAP(const MCCConfig& cfg, const URL& url, int timeout);
      MCC_Status process(PayloadSOAP *request, PayloadSOAP **response);
      MCC_Status process(const std::string& action, PayloadSOAP *request, PayloadSOAP **response);
      MCC* GetEntry();
      bool Load();
      MessageContext& GetContext();
      const Config& GetConfig() const;
    private:
      Config xmlcfg;
      MessageContext context;
  };

  Test_ClientSOAP::Test_ClientSOAP(const MCCConfig& cfg, const URL& url, int timeout) {}

  MCC_Status Test_ClientSOAP::process(PayloadSOAP *request, PayloadSOAP **response) {
      return MCC_Status(STATUS_OK);
  }
 
  MCC_Status Test_ClientSOAP::process(const std::string& action, PayloadSOAP *request, PayloadSOAP **response) {
      Arc::NS query_ns;
      query_ns[""] = "http://www.nordugrid.org/schemas/isis/2007/06";
      *response = new Arc::PayloadSOAP(query_ns);
      Arc::XMLNode r = (**response).NewChild("QueryResourcePropertiesResponse");
      r = "Test value";
      #ifdef NOT
        return MCC_Status(STATUS_UNDEFINED); 
      #endif
      return MCC_Status(STATUS_OK); 
  }

   MCC* Test_ClientSOAP::GetEntry() {
       return NULL;
   }

   bool Test_ClientSOAP::Load() {
       return false;
   }

   MessageContext& Test_ClientSOAP::GetContext() {
       return context;
   }

   const Config& Test_ClientSOAP::GetConfig() const {
       return xmlcfg;
   }


}

#endif

