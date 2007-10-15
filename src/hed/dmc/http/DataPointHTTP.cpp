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
    c.NewChild("ModuleManager");
    c["ModuleManager"].NewChild("Path");
    c["ModuleManager"].NewChild("Path");
    c["ModuleManager"]["Path"][0] =
      "../../hed/mcc/tcp/.libs";
    c["ModuleManager"]["Path"][1] =
      "../../hed/mcc/http/.libs";
    c.NewChild("Plugins");
    c.NewChild("Plugins");
    c["Plugins"][0].NewChild("Name");
    c["Plugins"][0]["Name"] = "mcctcp";
    c["Plugins"][1].NewChild("Name");
    c["Plugins"][1]["Name"] = "mcchttp";
    c.NewChild("Chain");
    c["Chain"].NewChild("Component");
    c["Chain"].NewChild("Component");
    c["Chain"]["Component"][0].NewAttribute("name");
    c["Chain"]["Component"][0].Attribute("name") = "tcp.client";
    c["Chain"]["Component"][0].NewAttribute("id");
    c["Chain"]["Component"][0].Attribute("id") = "tcp";
    c["Chain"]["Component"][0].NewChild("Connect");
    c["Chain"]["Component"][0]["Connect"].NewChild("Host");
    c["Chain"]["Component"][0]["Connect"]["Host"] =
      url.Host();
    c["Chain"]["Component"][0]["Connect"].NewChild("Port");
    c["Chain"]["Component"][0]["Connect"]["Port"] =
      tostring(url.Port());
    c["Chain"]["Component"][1].NewAttribute("name");
    c["Chain"]["Component"][1].Attribute("name") = "http.client";
    c["Chain"]["Component"][1].NewAttribute("id");
    c["Chain"]["Component"][1].Attribute("id") = "http";
    c["Chain"]["Component"][1].NewAttribute("entry");
    c["Chain"]["Component"][1].Attribute("entry") = "http";
    c["Chain"]["Component"][1].NewChild("next");
    c["Chain"]["Component"][1]["next"].NewAttribute("id");
    c["Chain"]["Component"][1]["next"].Attribute("id") = "tcp";
    c["Chain"]["Component"][1].NewChild("Method");
    c["Chain"]["Component"][1]["Method"] = "GET";
    c["Chain"]["Component"][1].NewChild("Endpoint");
    c["Chain"]["Component"][1]["Endpoint"] = url.str();

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
    f->SetType(FileInfo::file_type_file);
    f->SetSize(stringtoull(response.Attributes()->get("HTTP:content-length")));
    f->SetCreated(response.Attributes()->get("HTTP:last-modified"));
    return true;
  }

} // namespace Arc
