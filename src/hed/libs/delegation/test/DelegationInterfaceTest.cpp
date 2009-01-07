#include <sstream>

#include <cppunit/extensions/HelperMacros.h>

#include <arc/message/PayloadSOAP.h>
#include <arc/delegation/DelegationInterface.h>
 	
class DelegationInterfaceTest
  : public CppUnit::TestFixture {

  CPPUNIT_TEST_SUITE(DelegationInterfaceTest);
  CPPUNIT_TEST(TestDelegationInterfaceDELEGATE);
  CPPUNIT_TEST_SUITE_END();

public:
  void setUp();
  void tearDown();

  void TestDelegationInterfaceDELEGATE();

private:
  std::string credentials;
};

class DirectMCC: public Arc::MCCInterface {
  private:
    Arc::DelegationContainerSOAP& container_;
  public:
    DirectMCC(Arc::DelegationContainerSOAP& container):container_(container) {};
    Arc::MCC_Status process(Arc::Message& in,Arc::Message& out);
};

Arc::MCC_Status DirectMCC::process(Arc::Message& in,Arc::Message& out) {
  if(!in.Payload()) return Arc::MCC_Status();
  Arc::PayloadSOAP* in_payload = NULL;
  try {
    in_payload = dynamic_cast<Arc::PayloadSOAP*>(in.Payload());
  } catch(std::exception& e) { };
  if(!in_payload) return Arc::MCC_Status();
  Arc::MCC_Status r;
  Arc::NS ns;
  Arc::PayloadSOAP* out_payload = new Arc::PayloadSOAP(ns);
  out.Payload(out_payload);
  if((*in_payload)["DelegateCredentialsInit"]) {
    if(!container_.DelegateCredentialsInit(*in_payload,*out_payload)) return Arc::MCC_Status();
  } else if((*in_payload)["UpdateCredentials"]) {
    std::string cred;
    if(!container_.UpdateCredentials(cred,*in_payload,*out_payload)) return Arc::MCC_Status();
    if(cred.empty()) return Arc::MCC_Status();
  };
  return Arc::MCC_Status(Arc::STATUS_OK);
}

void DelegationInterfaceTest::setUp() {
  credentials.assign("\
-----BEGIN CERTIFICATE-----\n\
MIIBxjCCAS8CARAwDQYJKoZIhvcNAQEFBQAwKzENMAsGA1UEChMER3JpZDENMAsG\n\
A1UEChMEVGVzdDELMAkGA1UEAxMCQ0EwHhcNMDkwMTA1MTExNzQ2WhcNMDkwMjA0\n\
MTExNzQ2WjAsMQswCQYDVQQGEwJYWDEOMAwGA1UEChMFRHVtbXkxDTALBgNVBAMT\n\
BFRlc3QwgZ8wDQYJKoZIhvcNAQEBBQADgY0AMIGJAoGBAMPRUsusZTJG5tph8EUv\n\
s8Lvsv8+JRyoMuNhxcg5sy2MtxKvs1LBG8uBIeiI5vDHEyaA+kM3RP6/RvBD9Uru\n\
/qZRkmMlYwWDNyhU2Ft/7//M8jVIXl8pagWEwEAFwUPcdBX5OSPe5GFpeJnGtyWu\n\
0vLTrxDammqIDtdyrJM8c8AvAgMBAAEwDQYJKoZIhvcNAQEFBQADgYEAMNxlpMJo\n\
vo/2Mdwib+DLLyALm7HT0PbAFupj+QIyAntqqqQKaQqZwD4WeQf4jk2Vx9gGzFLV\n\
gEt3slFV2uxcuLf8BNQdPcv5rFwvwYu5AlExVZDUCQ06oR+RWiktekDWTAsx/PEt\n\
AjVVi0njg0Iev5AN7zWqxCOPjSW2yePNzCE=\n\
-----END CERTIFICATE-----\n\
-----BEGIN RSA PRIVATE KEY-----\n\
MIICXQIBAAKBgQDD0VLLrGUyRubaYfBFL7PC77L/PiUcqDLjYcXIObMtjLcSr7NS\n\
wRvLgSHoiObwxxMmgPpDN0T+v0bwQ/VK7v6mUZJjJWMFgzcoVNhbf+//zPI1SF5f\n\
KWoFhMBABcFD3HQV+Tkj3uRhaXiZxrclrtLy068Q2ppqiA7XcqyTPHPALwIDAQAB\n\
AoGAMuSPeUH4FyYYT7/Om5y3Qr3brrzvFlCc0T4TEmP0rqz409F4cNShrbWFI5OZ\n\
OhDzaDlzUc7mjrMV89IlyXDuG8WJJApCvd5fkZcigxa+cmrcGKRO/BOq5Zit0yKM\n\
ebE9csJKfj5WeXyjtQSWmAXlJJ5Y9bKO+PuVUaav5V/W/QkCQQDi33mOaf99o1o1\n\
jjnSUl5TgbqAtE4LXgnOgVl+Sazq3dVHBBhaFTzFYfa57YlvN8i6nYF8SfegpFJa\n\
Pt7BdSFlAkEA3PUrZgZDQDdrIFrqk12tW7P4YGqPkSjGrbuLTwGxhTiWhPL7Tej7\n\
Up/z8zpahDbGEXhNUgKKw0AOwHtZ2wssAwJBAJyPr2jyCRgApH4U2h4kLWffPH8Y\n\
7kq16HqTlNirqyKhV08cqllwEnH7+rGwFImlq2Xsz7Cfsr0u6I3SmRJT7GkCQQCJ\n\
v8q7gyH/8cy2Uhe1oYwHBI7OxQAV9f7OpoH10k9yh1HHNpgW/S1ZWGDEfNebX25h\n\
y8cgXndVvCS2OPBOz4szAkBWx+6KgpQ+Xdx5Jv7IoGRdE9GdIGMtTaHOnUxSsdlj\n\
buEHRt+0Gp5Rod9S6w9Ppl6CphSPq5HRCo49SBBRgAWm\n\
-----END RSA PRIVATE KEY-----\n\
");
}

void DelegationInterfaceTest::tearDown() {
}

void DelegationInterfaceTest::TestDelegationInterfaceDELEGATE() {
  Arc::DelegationContainerSOAP c;
  Arc::DelegationProviderSOAP p(credentials);
  DirectMCC m(c);
  Arc::MessageContext context;

  CPPUNIT_ASSERT((bool)p.DelegateCredentialsInit(m,&context));
  CPPUNIT_ASSERT((bool)p.UpdateCredentials(m,&context));
}

CPPUNIT_TEST_SUITE_REGISTRATION(DelegationInterfaceTest);


