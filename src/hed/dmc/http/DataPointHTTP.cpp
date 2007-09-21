#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <arc/ArcConfig.h>
#include <arc/Logger.h>
#include <arc/StringConv.h>
#include <arc/loader/Loader.h>
#include <arc/message/PayloadRaw.h>
#include <arc/data/DataBufferPar.h>
#include "DataPointHTTP.h"

namespace Arc {

  Logger DataPointHTTP::logger(DataPoint::logger, "HTTP");

  DataPointHTTP::DataPointHTTP(const URL& url) : DataPointDirect(url) {}

  DataPointHTTP::~DataPointHTTP() {}

  bool DataPointHTTP::list_files(std::list<FileInfo>& files, bool) {
    Arc::NS ns;
    Arc::Config c(ns);
    c.NewChild("ArcConfig");
    c["ArcConfig"].NewChild("ModuleManager");
    c["ArcConfig"]["ModuleManager"].NewChild("Path");
    c["ArcConfig"]["ModuleManager"].NewChild("Path");
    c["ArcConfig"]["ModuleManager"]["Path"][0] =
      "../../hed/mcc/tcp/.libs";
    c["ArcConfig"]["ModuleManager"]["Path"][1] =
      "../../hed/mcc/http/.libs";
    c["ArcConfig"].NewChild("Plugins");
    c["ArcConfig"].NewChild("Plugins");
    c["ArcConfig"]["Plugins"][0].NewChild("Name");
    c["ArcConfig"]["Plugins"][0]["Name"] = "mcctcp";
    c["ArcConfig"]["Plugins"][1].NewChild("Name");
    c["ArcConfig"]["Plugins"][1]["Name"] = "mcchttp";
    c["ArcConfig"].NewChild("Chain");
    c["ArcConfig"]["Chain"].NewChild("Component");
    c["ArcConfig"]["Chain"].NewChild("Component");
    c["ArcConfig"]["Chain"]["Component"][0].NewAttribute("name");
    c["ArcConfig"]["Chain"]["Component"][0].Attribute("name") = "tcp.client";
    c["ArcConfig"]["Chain"]["Component"][0].NewAttribute("id");
    c["ArcConfig"]["Chain"]["Component"][0].Attribute("id") = "tcp";
    c["ArcConfig"]["Chain"]["Component"][0].NewChild("Connect");
    c["ArcConfig"]["Chain"]["Component"][0]["Connect"].NewChild("Host");
    c["ArcConfig"]["Chain"]["Component"][0]["Connect"]["Host"] =
      url.Host();
    c["ArcConfig"]["Chain"]["Component"][0]["Connect"].NewChild("Port");
    c["ArcConfig"]["Chain"]["Component"][0]["Connect"]["Port"] =
      tostring(url.Port());
    c["ArcConfig"]["Chain"]["Component"][1].NewAttribute("name");
    c["ArcConfig"]["Chain"]["Component"][1].Attribute("name") = "http.client";
    c["ArcConfig"]["Chain"]["Component"][1].NewAttribute("id");
    c["ArcConfig"]["Chain"]["Component"][1].Attribute("id") = "http";
    c["ArcConfig"]["Chain"]["Component"][1].NewAttribute("entry");
    c["ArcConfig"]["Chain"]["Component"][1].Attribute("entry") = "http";
    c["ArcConfig"]["Chain"]["Component"][1].NewChild("next");
    c["ArcConfig"]["Chain"]["Component"][1]["next"].NewAttribute("id");
    c["ArcConfig"]["Chain"]["Component"][1]["next"].Attribute("id") = "tcp";
    c["ArcConfig"]["Chain"]["Component"][1].NewChild("Method");
    c["ArcConfig"]["Chain"]["Component"][1]["Method"] = "GET";
    c["ArcConfig"]["Chain"]["Component"][1].NewChild("Endpoint");
    c["ArcConfig"]["Chain"]["Component"][1]["Endpoint"] = url.str();

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

    std::list<FileInfo>::iterator f = files.insert(files.end(), url.Path());
    f->type = FileInfo::file_type_file;
    f->size = stringtoull(response.Attributes()->get("HTTP:content-length"));
    f->created = response.Attributes()->get("HTTP:last-modified");
    return true;
  }

} // namespace Arc
