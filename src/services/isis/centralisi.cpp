#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/loader/Loader.h>
#include <arc/loader/ServiceLoader.h>
#include <arc/message/PayloadSOAP.h>
#include <arc/ws-addressing/WSA.h>
#include <arc/URL.h>
#include <arc/Thread.h>
#include <glibmm.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <fstream>
#ifdef WIN32
#include <arc/win32.h>
#endif

#include "centralisi.h"

namespace CentralISI {

static Arc::Service *get_service(Arc::Config *cfg, Arc::ChainContext *) { 
    return new CentralISIService(cfg);
}

static void infocollector_thread(void *arg)
{
    if (!arg) return;
    CentralISIService *srv = (CentralISIService *)arg;
    srv->InformationCollector();
}

static void register_thread(void *arg)
{
    if (!arg) return;
    CentralISIService *srv = (CentralISIService *)arg;
    srv->reg->registration_forever();
}

static void MakeISIFault(Arc::XMLNode fault)
{
    fault.NewChild("isis:Name")="undefined";
    fault.NewChild("isis:Description");
    fault.NewChild("isis:Type")="1";
}

void CentralISIService::InformationCollector(void)
{
    for (;;) {
        logger.msg(Arc::DEBUG, "Information collector");
        // Create infodoc
        Arc::NS ns;
        Arc::XMLNode node(ns, "InfoDoc");
        // TODO: make rest Glue2
        Arc::XMLNode service_node = node.NewChild("Service");
        service_node.NewChild("ID") = service_id;
        service_node.NewChild("Name") = "CentralISIService";
        service_node.NewChild("Type") = "InformationIndexingService";
        Arc::XMLNode ep = service_node.NewChild("Endpoint");
        ep.NewChild("ID") = "0";
        ep.NewChild("URL") = "http://localhost:60000/isis";
        // Write data to info cache
        if(icache) icache->Cache().Set("/local", node);
        sleep(60); // XXX: make it configurable
    }
}

CentralISIService::CentralISIService(Arc::Config *cfg):Service(cfg),logger(Arc::Logger::rootLogger, "CentralISI"),reg(NULL)
{
    // Define supported namespaces
    // TODO: Remove those addedd automatically and take others from headers
    ns["isis"]="http://www.nordugrid.org/schemas/isis/2007/06";
    ns["wsa"]="http://www.w3.org/2005/08/addressing";
    ns["wsrf-bf"]="http://docs.oasis-open.org/wsrf/bf-2";
    ns["wsrf-rp"]="http://docs.oasis-open.org/wsrf/rp-2";
    ns["wsrf-rpw"]="http://docs.oasis-open.org/wsrf/rpw-2";
    ns["wsrf-rw"]="http://docs.oasis-open.org/wsrf/rw-2";
    
    logger.msg(Arc::DEBUG, "Central ISIS initialized");
    /*
    std::string dsn = (std::string)((*cfg)["DSN"]);
    if(dsn.empty()) {
        logger.msg(Arc::ERROR, "Missing database location in configuration");
    } else {
        logger.msg(Arc::VERBOSE, "Database location is %s",dsn);
        Arc::URL dsn_url(dsn);
        if ((dsn_url.Protocol() != "file") ||
            ((!dsn_url.Host().empty()) && (dsn_url.Host() != "localhost"))) {
            logger.msg(Arc::ERROR, "remote database not supported");
        } else {
            logger.msg(Arc::VERBOSE, "Database URL is %s",dsn_url.str());
            logger.msg(Arc::VERBOSE, "Database path is %s",dsn_url.Path());
            logger.msg(Arc::VERBOSE, "Database host is %s",dsn_url.Host());
            db_path = dsn_url.Path();
            logger.msg(Arc::INFO, "Initializing database %s", db_path);
            if (!Glib::file_test(db_path, Glib::FILE_TEST_IS_DIR)) {
                // create root directory
                if (mkdir(db_path.c_str(), 0700) != 0) 
                {
                    logger.msg(Arc::ERROR, "cannot create directory: %s", db_path);
                    db_path = "";
                }        
            }        
        }
    }
    */
    Arc::Config regx((*cfg)["Register"]);
    service_id = (std::string)(*cfg)["ID"];
    if(service_id.empty()) service_id="ISIS";
    Arc::Config icfg((*cfg)["InformationSystem"]);
    icache = new Arc::InfoCacheInterface(icfg, service_id);
    mcache = new Arc::InfoCache(icfg, service_id+".meta");
    CreateThreadFunction(&infocollector_thread, this);
    if(!regx) {
        logger.msg(Arc::WARNING, "Missing registration section, won't register anywhere");
    } else {
        std::string s_reg_period = regx["Period"];
        long int reg_period = strtol(s_reg_period.c_str(), NULL, 10);
        if(reg_period == 0) reg_period=600;
        reg = new Arc::InfoRegister(service_id, reg_period, regx);
        reg->AddUrl("http://localhost:60000/isis");
        CreateThreadFunction(&register_thread, this);
    };
}

CentralISIService::~CentralISIService(void)
{
    // delete reg;
    if(icache) delete icache;
}

Arc::MCC_Status CentralISIService::process(Arc::Message &inmsg, Arc::Message &outmsg)
{
    logger.msg(Arc::DEBUG, "process called");
    /*
    if (db_path == "") {
        logger.msg(Arc::ERROR, "invalid database path");
        return Arc::MCC_Status();
    }
    */
    // Both input and output are supposed to be SOAP
    // Extracting payload
    Arc::PayloadSOAP* inpayload = NULL;
    try {
      inpayload = dynamic_cast<Arc::PayloadSOAP*>(inmsg.Payload());
    } catch(std::exception& e) { };
    if(!inpayload) {
      logger.msg(Arc::ERROR, "input is not SOAP");
      return make_soap_fault(outmsg);
    };
    // Analyzing request
    Arc::XMLNode op = inpayload->Child(0);
    if(!op) {
      logger.msg(Arc::ERROR, "input does not define operation");
      return make_soap_fault(outmsg);
    }; 
    std::string x_str;
    op.GetXML(x_str);
    logger.msg(Arc::VERBOSE, "SOAP request: %s",x_str);
    logger.msg(Arc::DEBUG, "process: operation: %s", op.Name());
    Arc::PayloadSOAP* outpayload = new Arc::PayloadSOAP(ns);
    Arc::PayloadSOAP& res = *outpayload;
    Arc::MCC_Status ret = Arc::STATUS_OK;
    if(MatchXMLName(op, "Register")) {
        Arc::XMLNode r = res.NewChild("isis:RegisterResponse");
        ret = Register(op, r);
    } else if(MatchXMLName(op, "RemoveRegistrations")) {
        Arc::XMLNode r = res.NewChild("isis:RemoveRegistrationsResponse");
        ret = RemoveRegistrations(op, r);
    } else if(MatchXMLName(op, "GetRegistrationStatuses")) {
        Arc::XMLNode r = res.NewChild("isis:GetRegistrationStatusesResponse");
        ret = GetRegistrationStatuses(op, r);
    } else if(MatchXMLName(op, "GetISISList")) {
        Arc::XMLNode r = res.NewChild("isis:GetISISListResponse");
        ret = GetISISList(op, r);
    /* This operation is not defined 
    } else if(MatchXMLName(op, "Get")) {
        Arc::XMLNode r = res.NewChild("isis:GetResponse");
        ret = Get(op, r);
    */
    /* This operation is not defined 
    } else if(MatchXMLName(op, "DelegateCredentialsInit")) {
        if(!delegation.DelegateCredentialsInit(*inpayload,*outpayload)) {
          delete inpayload;
          return make_soap_fault(outmsg);
        }
    */
    // WS-Property
    } else if(MatchXMLNamespace(op, "http://docs.oasis-open.org/wsrf/rp-2") && icache) {
        Arc::SOAPEnvelope* out_ = icache->Process(*inpayload);
        if(out_) {
          *outpayload=*out_;
          delete out_;
        } else {
          delete inpayload; delete outpayload;
          return make_soap_fault(outmsg);
        };
    } else {
        logger.msg(Arc::ERROR,"SOAP operation is not supported: %s", op.Name());
        return make_soap_fault(outmsg);
    };
    // Set output
    outpayload->GetXML(x_str);
    logger.msg(Arc::VERBOSE, "SOAP response: %s",x_str);
    outmsg.Payload(outpayload);
    return Arc::MCC_Status(ret);
}

Arc::MCC_Status CentralISIService::make_soap_fault(Arc::Message& outmsg)
{
    Arc::PayloadSOAP* outpayload = new Arc::PayloadSOAP(ns,true);
    Arc::SOAPFault* fault = outpayload?outpayload->Fault():NULL;
    if(fault) {
        fault->Code(Arc::SOAPFault::Sender);
        fault->Reason("Failed processing request");
    };
    outmsg.Payload(outpayload);
    return Arc::MCC_Status(Arc::STATUS_OK);
}



static void url_encode(std::string& s) {
  for(std::string::size_type n=0;n<s.size();++n) {
    if(isalnum(s[n])) continue;
    char buf[4];
    snprintf(buf,4,"%%%02x",(unsigned int)(s[n]));
    buf[3]=0;
    s.replace(n,1,buf,3); n+=2;
  };
}

Arc::MCC_Status CentralISIService::Register(Arc::XMLNode &in, Arc::XMLNode &out)
{
    int i;
    Arc::XMLNode entry;
    bool failed = false;
    for (i = 0; (bool)(entry = in["RegEntry"][i]); i++) {
        // Meaning of element ID is undefined. Service is supposed to be 
        // identified by EPR.
        // std::string id = (std::string)entry["ID"];
        std::string id = Arc::WSAEndpointReference(entry["SrcAdv"]["EPR"]).Address();
        if(id.empty()) {
            // logger.msg(Arc::ERROR, "Missing identifier in registration request");
            logger.msg(Arc::ERROR, "Missing address in registering service EPR");
            failed=true; continue;
        }
        // Converting id into safe string
        url_encode(id);
        Arc::XMLNode adv = entry["SrcAdv"];
        if(!adv) {
            logger.msg(Arc::ERROR, "Missing service advertisement in registering service");
            failed=true; continue;
        }
        Arc::XMLNode madv = entry["MetaSrcAdv"];
        if(!madv) {
            logger.msg(Arc::ERROR, "Missing service meta advertisement in registering service");
            failed=true; continue;
        }
        if(!icache) {
            failed=true; continue;
        }
        if(!mcache) {
            failed=true; continue;
        }
        Arc::XMLNode infodoc(ns,"InfoDoc");
        infodoc.NewChild(adv);
        failed|=!(icache->Cache().Set("/"+id,infodoc));
        failed|=!(mcache->Set("/"+id,madv));
        /*
        std::string data;
        entry.GetXML(data);
        std::string entry_file = Glib::build_filename(db_path, id);
        std::ofstream out(entry_file.c_str(), std::ios::out);
        if (!out) {
            continue;
        }
        out << "<?xml version=\"1.0\" encoding=\"utf-8\"?>" << std::endl;
        out << data;
        out.close();
        */
    }
    if(failed) {
      logger.msg(Arc::ERROR, "Register: reporting fault");
      MakeISIFault(out.NewChild("isis:Fault"));
    };
    return Arc::MCC_Status(Arc::STATUS_OK);
}

Arc::MCC_Status CentralISIService::RemoveRegistrations(Arc::XMLNode &in, Arc::XMLNode &/*out*/)
{
    int i;
    Arc::XMLNode entry;
    for (i = 0; (entry = in["RegEntryID"][i]) == true; i++) {
        std::string id = Arc::WSAEndpointReference(entry).Address();
        if(id.empty()) {
            logger.msg(Arc::ERROR, "Missing address in unregistering service EPR");
            continue;
        }
        url_encode(id);
        if(icache) icache->Cache().Unset("/"+id);
        if(mcache) mcache->Unset("/"+id);
        /*
        std::string path = Glib::build_filename(db_path, id);
        unlink(path.c_str());
        */
    }
    return Arc::MCC_Status(Arc::STATUS_OK);
}

Arc::MCC_Status CentralISIService::GetRegistrationStatuses(Arc::XMLNode &in, Arc::XMLNode &out)
{
    int i;
    Arc::XMLNode entry;
    for (i = 0; (entry = in["RegEntryID"][i]) == true; i++) {
        std::string id = Arc::WSAEndpointReference(entry).Address();
        Arc::XMLNode re = out.NewChild("isis:RegEntryStatus");
        if(id.empty()) {
            logger.msg(Arc::ERROR, "Missing address in service EPR");
            continue;
        } else {
          url_encode(id);
          if(mcache) {
            Arc::XMLNodeContainer result;
            if(mcache->Get("/"+id,result)) {
              if(result.Size() >= 1) {
                Arc::XMLNode status = result[0]["Status"];
                if(status) {
                  re.NewChild(entry);     // isis:RegEntryID
                  re.NewChild("isis:ID"); // isis::ID
                  re.NewChild(status);    // isis::Status
                };
              };
            };
          };
        };
        if(re.Size() == 0) {
          MakeISIFault(re.NewChild("isis:FaultType"));
        };
        /*
        std::string path = Glib::build_filename(db_path, id);
        if (Glib::file_test(path, Glib::FILE_TEST_EXISTS) == true) {
            std::string data = Glib::file_get_contents(path);
            Arc::XMLNode d(data);
            Arc::XMLNode status = d["MetaSrcAdv"]["Status"];
            if(status == true) {
                out.NewChild("RegEntry");
                out["RegEntry"].NewChild(entry);
                out["RegEntry"].NewChild(status);
            }
        }
        */
    }
    std::string str;
    out.GetXML(str);
    return Arc::MCC_Status(Arc::STATUS_OK);
}

Arc::MCC_Status CentralISIService::GetISISList(Arc::XMLNode &/*in*/, Arc::XMLNode &/*out*/)
{
    return Arc::MCC_Status();
}

Arc::MCC_Status CentralISIService::Get(Arc::XMLNode &/*in*/, Arc::XMLNode &/*out*/)
{
    return Arc::MCC_Status();
}

} // namespace CentralISI

service_descriptors ARC_SERVICE_LOADER = {
    { "centralisi", 0, &CentralISI::get_service },
    { NULL, 0, NULL }
};
