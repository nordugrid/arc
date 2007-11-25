#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/Logger.h>
#include <fstream>
#include <glibmm.h>
#include "iic.h"

int main(int argc, char **argv)
{
    Arc::LogStream logcerr(std::cerr, "ISISClient");
    Arc::Logger::getRootLogger().addDestination(logcerr);
    if (argc < 4) {
        std::cerr << "Invalid arguments!" << std::endl;
        return -1;
    }
    
    std::string op = argv[1];
    std::string url = argv[2];
    if (op == "reg") {
        std::string xml_str = Glib::file_get_contents(argv[3]);
        Arc::XMLNode req(xml_str);
        Arc::IIClient cli(url);
        Arc::XMLNode *resp = new Arc::XMLNode();
        Arc::MCC_Status status = cli.Register(req, resp);
        std::string out;
        resp->GetXML(out);
        std::cout << "Status: " << status << std::endl;
        std::cout << "Response: " << out << std::endl;
    } else if (op == "rm") {
        std::string xml_str = Glib::file_get_contents(argv[3]);
        Arc::XMLNode req(xml_str);
        Arc::IIClient cli(url);
        Arc::XMLNode *resp = new Arc::XMLNode();
        Arc::MCC_Status status = cli.RemoveRegistrations(req, resp);
        std::string out;
        resp->GetXML(out);
        std::cout << "Status: " << status << std::endl;
        std::cout << "Response: " << out << std::endl;
    } else if (op == "status") {
        std::string xml_str = Glib::file_get_contents(argv[3]);
        Arc::XMLNode req(xml_str);
        Arc::IIClient cli(url);
        Arc::XMLNode *resp = new Arc::XMLNode();
        Arc::MCC_Status status = cli.GetRegistrationStatuses(req, resp);
        std::string out;
        resp->GetXML(out);
        std::cout << "Status: " << status << std::endl;
        std::cout << "Response: " << out << std::endl;
    }
    return 0;
}
