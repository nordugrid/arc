#ifndef __ARC_AREXCLIENTTEST_H__
#define __ARC_AREXCLIENTTEST_H__

#ifdef ClientSOAP
#undef ClientSOAP
#endif

namespace Arc {
class ClientSOAPTest :
  public ClientSOAP {
  public:
    /** Constructor creates MCC chain and connects to server. */
    ClientSOAPTest()
      : ClientSOAP() {}
    ClientSOAPTest(const BaseConfig& cfg, const URL& url, int timeout = -1)
      : ClientSOAP(cfg, url, timeout) {}

    /** Send SOAP request and receive response. */
    MCC_Status process(PayloadSOAP *request, PayloadSOAP **response);
    /** Send SOAP request with specified SOAP action and receive response. */
    MCC_Status process(const std::string& action, PayloadSOAP *request,
                            PayloadSOAP **response);
};
}

#ifndef ClientSOAP
#define ClientSOAP ClientSOAPTest
#endif

#endif // __ARC_AREXCLIENTTEST_H__
