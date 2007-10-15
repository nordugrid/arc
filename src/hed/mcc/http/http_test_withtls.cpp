#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string>
#include <iostream>

#include <arc/loader/Loader.h>
#include <arc/message/PayloadRaw.h>
#include <arc/ArcConfig.h>
#include <arc/StringConv.h>
#include <arc/URL.h>

void test1(void) {
  std::cout<<"------ Testing simple file download ------"<<std::endl;

  Arc::URL url("https://download.nordugrid.org/index.html");

  Arc::NS ns;
  Arc::Config c(ns);
  Arc::XMLNode cfg = c;
  Arc::XMLNode mgr = cfg.NewChild("ModuleManager");
  Arc::XMLNode pth1 = mgr.NewChild("Path");
  pth1 = "../tcp/.libs";
  Arc::XMLNode pth2 = mgr.NewChild("Path");
  pth2 = "../tls/.libs";
  Arc::XMLNode pth3 = mgr.NewChild("Path");
  pth3 = ".libs";
  Arc::XMLNode plg1 = cfg.NewChild("Plugins");
  Arc::XMLNode mcctcp = plg1.NewChild("Name");
  mcctcp = "mcctcp";
  Arc::XMLNode plg2 = cfg.NewChild("Plugins");
  Arc::XMLNode mcctls = plg2.NewChild("Name");
  mcctls = "mcctls";
  Arc::XMLNode plg3 = cfg.NewChild("Plugins");
  Arc::XMLNode mcchttp = plg3.NewChild("Name");
  mcchttp = "mcchttp";
  Arc::XMLNode chn = cfg.NewChild("Chain");

  Arc::XMLNode tcp = chn.NewChild("Component");
  Arc::XMLNode tcpname = tcp.NewAttribute("name");
  tcpname = "tcp.client";
  Arc::XMLNode tcpid = tcp.NewAttribute("id");
  tcpid = "tcp";
  Arc::XMLNode tcpcnt = tcp.NewChild("Connect");
  Arc::XMLNode tcphost = tcpcnt.NewChild("Host");
  tcphost = url.Host();
  Arc::XMLNode tcpport = tcpcnt.NewChild("Port");
  tcpport = Arc::tostring(url.Port());

  Arc::XMLNode tls = chn.NewChild("Component");
  Arc::XMLNode tlsname = tls.NewAttribute("name");
  tlsname = "tls.client";
  Arc::XMLNode tlsid = tls.NewAttribute("id");
  tlsid = "tls";
  Arc::XMLNode tlsnext = tls.NewChild("next");
  Arc::XMLNode tlsnextid = tlsnext.NewAttribute("id");
  tlsnextid = "tcp";

  Arc::XMLNode http = chn.NewChild("Component");
  Arc::XMLNode httpname = http.NewAttribute("name");
  httpname = "http.client";
  Arc::XMLNode httpid = http.NewAttribute("id");
  httpid = "http";
  Arc::XMLNode httpentry = http.NewAttribute("entry");
  httpentry = "http";
  Arc::XMLNode httpnext = http.NewChild("next");
  Arc::XMLNode httpnextid = httpnext.NewAttribute("id");
  httpnextid = "tls";
  Arc::XMLNode httpmeth = http.NewChild("Method");
  httpmeth = "GET";
  Arc::XMLNode httpep = http.NewChild("Endpoint");
  httpep = url.str();

  std::cout<<"------ Configuration ------"<<std::endl;
  std::string cfgstr;
  c.GetXML(cfgstr);
  std::cerr << cfgstr << std::endl;

  Arc::Loader l(&c);

  Arc::Message request;
  Arc::PayloadRaw msg;
  Arc::MessageAttributes attributes;
  Arc::MessageContext context;
  request.Payload(&msg);
  request.Attributes(&attributes);
  request.Context(&context);
  Arc::Message response;

  l["http"]->process(request,response);

  std::cout<<"*** RESPONSE ***"<<std::endl;
  Arc::PayloadRaw& payload =
    dynamic_cast<Arc::PayloadRaw&>(*response.Payload());
  for(int n = 0;n<payload.Size();++n) std::cout<<payload[n];
  std::cout<<std::endl;
}

int main(void) {
  test1();
  return 0;
}
