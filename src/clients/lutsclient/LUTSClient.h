#include <stdexcept>
#include <string>
#include <map>
#include <arc/XMLNode.h>
#include <arc/client/ClientInterface.h>
#include <arc/message/MCC.h>
#include <arc/message/MCCLoader.h>
#include <arc/message/Message.h>
#include <arc/message/PayloadSOAP.h>

namespace Arc
{
  /**
   * Class to use LUTS functionalities from client side.
   */
  class LUTSClient
  {
    //Arc::URL endpoint_url;
    Arc::MCCLoader *mccloader;
    Arc::MCC* soapmcc;
    Arc::Config clientchain;
  public:
    LUTSClient(Arc::Config &cfg);
    Arc::MCC_Status log_urs(const std::string &urset);
  };
}
