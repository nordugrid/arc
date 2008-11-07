#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/Logger.h>
#include <fstream>
#include <glibmm.h>
#include <arc/infosys/InfoRegister.h>

#include "iic.h"

int main(int argc, char **argv)
{
    Arc::LogStream logcerr(std::cerr);
    Arc::Logger::getRootLogger().addDestination(logcerr);
    if (argc < 4) {
        std::cerr << "Invalid arguments!" << std::endl;
        return -1;
    }
    Arc::NS ns;
    ns["isis"] = ISIS_NAMESPACE;
    ns["glue2"] = GLUE2_D42_NAMESPACE;
    std::string url = argv[1];
    std::string op = argv[2];
    if (op == "query") {
        Arc::XMLNode req(ns, "isis:Query");
        std::string q = argv[3];
        std::cout << "Query: " << q << std::endl;
        req.NewChild("isis:XPathQuery") = argv[3];
        Arc::IIClient cli(url);
        Arc::XMLNode *resp = new Arc::XMLNode();
        Arc::MCC_Status status = cli.Query(req, resp);
        std::string out;
        resp->GetXML(out);
        std::cout << "Status: " << status << std::endl;
        std::cout << "Response: " << out << std::endl;
    }
    return 0;
}
