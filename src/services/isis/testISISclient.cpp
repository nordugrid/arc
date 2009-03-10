/* Test ISIS client for the following operations:
    Query - for querying remote ISIS
        attributes: The query string.
    Register - for sending test Register messages
        attributes:
            The ServiceID for register.
            A key - value pair for register.
    RemoveRegistration - for sending test RemoveRegistration messages
        attributes: The ServiceID for remove.
    (The GetISISList operation will be used in every other cases impicitly.)

    Usage:
        testISISclient Query "query string"
        testISISclient Register "ServiceID1,EPR1" "ServiceID2,EPR2" "ServiceID3,EPR3"
        testISISclient RemoveRegistration "ServiceID1" "ServiceID2"
        etc.

    Examples:
        testISISclient -m Register "Srv_ID1,EPR1"
        testISISclient -m Query "/RegEntry/MetaSrcAdv/ServiceID[text()=\"Srv_ID1\"]"
        testISISclient -m RemoveRegistration "Srv_ID1"

*/

#include <sys/stat.h>
#include <time.h>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <vector>

#include <arc/OptionParser.h>
#include <arc/IString.h>
#include <arc/ArcConfig.h>
#include <arc/Logger.h>
#include <arc/message/MCCLoader.h>
#include <arc/message/PayloadSOAP.h>
#include <arc/XMLNode.h>
#include <arc/URL.h>

Arc::Logger logger(Arc::Logger::rootLogger, "ISISTest");


std::string ChainConfigString( Arc::URL url ) {

    std::string host = url.Host();
    std::string port;
    std::stringstream ss;
    ss << url.Port();
    ss >> port;
    std::string path = url.Path();
    logger.msg(Arc::DEBUG, " [ host : %s ]", host);
    logger.msg(Arc::DEBUG, " [ port : %s ]", port);
    logger.msg(Arc::DEBUG, " [ path : %s ]", path);

    // Create client chain
    std::string doc="";
    doc +="\n";
    doc +="    <ArcConfig\n";
    doc +="      xmlns=\"http://www.nordugrid.org/schemas/ArcConfig/2007\"\n";
    doc +="      xmlns:tcp=\"http://www.nordugrid.org/schemas/ArcMCCTCP/2007\">\n";
    doc +="     <ModuleManager>\n";
    doc +="        <Path>.libs/</Path>\n";
    doc +="        <Path>../../hed/mcc/http/.libs/</Path>\n";
    doc +="        <Path>../../hed/mcc/soap/.libs/</Path>\n";
    doc +="        <Path>../../hed/mcc/tls/.libs/</Path>\n";
    doc +="        <Path>../../hed/mcc/tcp/.libs/</Path>\n";
    doc +="     </ModuleManager>\n";
    doc +="     <Plugins><Name>mcctcp</Name></Plugins>\n";
    doc +="     <Plugins><Name>mcctls</Name></Plugins>\n";
    doc +="     <Plugins><Name>mcchttp</Name></Plugins>\n";
    doc +="     <Plugins><Name>mccsoap</Name></Plugins>\n";
    doc +="     <Chain>\n";
    doc +="      <Component name='tcp.client' id='tcp'><tcp:Connect><tcp:Host>" + host + "</tcp:Host><tcp:Port>" + port + "</tcp:Port></tcp:Connect></Component>\n";
    doc +="      <Component name='http.client' id='http'><next id='tcp'/><Method>POST</Method><Endpoint>"+ path +"</Endpoint></Component>\n";
    doc +="      <Component name='soap.client' id='soap' entry='soap'><next id='http'/></Component>\n";
    doc +="     </Chain>\n";
    doc +="    </ArcConfig>";

    return doc;
}


// Query function
std::string Query( Arc::URL url, std::string query ){

    Arc::XMLNode client_doc(ChainConfigString(url));
    Arc::Config client_config(client_doc);
    if(!client_config) {
      logger.msg(Arc::ERROR, "Failed to load client configuration");
      return "-1";
    };
    Arc::MCCLoader client_loader(client_config);
    logger.msg(Arc::INFO, "Client side MCCs are loaded");
    Arc::MCC* client_entry = client_loader["soap"];
    if(!client_entry) {
      logger.msg(Arc::ERROR, "Client chain does not have entry point");
      return "-1";
    };

    // Create and send Query request
    logger.msg(Arc::INFO, "Creating and sending request");
    Arc::NS query_ns; query_ns["isis"]="urn:isis";
    query_ns[""] = "http://www.nordugrid.org/schemas/isis/2007/06";
    Arc::PayloadSOAP req(query_ns);

    Arc::XMLNode request = req.NewChild("Query");
    request.NewChild("QueryString") = query;
    Arc::Message reqmsg;
    Arc::Message repmsg;
    reqmsg.Payload(&req);
    // It is a responsibility of code initiating first Message to
    // provide Context and Attributes as well.
    Arc::MessageAttributes attributes_req;
    Arc::MessageAttributes attributes_rep;
    Arc::MessageContext context;
    reqmsg.Attributes(&attributes_req);
    reqmsg.Context(&context);
    repmsg.Attributes(&attributes_rep);
    repmsg.Context(&context);

    Arc::MCC_Status status;
    std::cout << " Request sent. Waiting for the response." << std::endl;
    status= client_entry->process(reqmsg,repmsg);

    if(!status) {
      logger.msg(Arc::ERROR, "Request failed");
      std::cerr << "Status: " << std::string(status) << std::endl;
      return "-1";
    };

    Arc::PayloadSOAP* resp = NULL;
    if(repmsg.Payload() == NULL) {
      logger.msg(Arc::ERROR, "There is no response");
      return "-1";
    };
    try {
      resp = dynamic_cast<Arc::PayloadSOAP*>(repmsg.Payload());
    } catch(std::exception&) { };
    if(resp == NULL) {
      logger.msg(Arc::ERROR, "Response is not SOAP");
      return "-1";
    };

    //The response message
    std::string response = "";
    if (bool((*resp)["QueryResponse"]) ){
          std::string xml;
          (*resp)["QueryResponse"].GetDoc(xml, true);
          response += xml;
          response +="\n";
    }

    return response;
}


// Register function
std::string Register( Arc::URL url, std::vector<std::string> &serviceID, std::vector<std::string> &epr ){

    if (serviceID.size() != epr.size()){
       logger.msg(Arc::DEBUG, " Service_ID's number is not equivalent with the ERP's number!");
       return "-1";
    }

    Arc::XMLNode client_doc(ChainConfigString(url));
    Arc::Config client_config(client_doc);
    if(!client_config) {
      logger.msg(Arc::ERROR, "Failed to load client configuration");
      return "-1";
    };
    Arc::MCCLoader client_loader(client_config);
    logger.msg(Arc::INFO, "Client side MCCs are loaded");
    Arc::MCC* client_entry = client_loader["soap"];
    if(!client_entry) {
      logger.msg(Arc::ERROR, "Client chain does not have entry point");
      return "-1";
    };

    // Create and send Register request
    logger.msg(Arc::INFO, "Creating and sending request");
    Arc::NS query_ns; query_ns["isis"]="urn:isis";
    query_ns[""] = "http://www.nordugrid.org/schemas/isis/2007/06";
    query_ns["wsa"] = "http://www.w3.org/2005/08/addressing";
    Arc::PayloadSOAP req(query_ns);

    Arc::XMLNode request = req.NewChild("Register");
    request.NewChild("Header");
    //request["Header"].NewChild("RequesterID");

    time_t rawtime;
    time ( &rawtime );	//current time
    tm * ptm;
    ptm = gmtime ( &rawtime );

    std::string mon_prefix = (ptm->tm_mon+1 < 10)?"0":"";
    std::string day_prefix = (ptm->tm_mday < 10)?"0":"";
    std::string hour_prefix = (ptm->tm_hour < 10)?"0":"";
    std::string min_prefix = (ptm->tm_min < 10)?"0":"";
    std::string sec_prefix = (ptm->tm_sec < 10)?"0":"";
    std::stringstream out;
    out << ptm->tm_year+1900<<"-"<<mon_prefix<<ptm->tm_mon+1<<"-"<<day_prefix<<ptm->tm_mday<<"T"<<hour_prefix<<ptm->tm_hour<<":"<<min_prefix<<ptm->tm_min<<":"<<sec_prefix<<ptm->tm_sec;
    request["Header"].NewChild("MessageGenerationTime") = out.str();

    for (int i=0; i < serviceID.size(); i++){
        Arc::XMLNode srcAdv = request.NewChild("RegEntry").NewChild("SrcAdv");
        srcAdv.NewChild("Type") = "org.nordugrid.test.testISISclient";
        Arc::XMLNode epr = srcAdv.NewChild("EPR");
        epr.NewChild("wsa:Address") = epr[i];
        //srcAdv.NewChild("SSPair");

        Arc::XMLNode metaSrcAdv = request["RegEntry"][i].NewChild("MetaSrcAdv");
        metaSrcAdv.NewChild("ServiceID") = serviceID[i];
        metaSrcAdv.NewChild("GenTime") = out.str();
        metaSrcAdv.NewChild("Expiration") = "P30M";
    }
    Arc::Message reqmsg;
    Arc::Message repmsg;
    reqmsg.Payload(&req);
    // It is a responsibility of code initiating first Message to
    // provide Context and Attributes as well.
    Arc::MessageAttributes attributes_req;
    Arc::MessageAttributes attributes_rep;
    Arc::MessageContext context;
    reqmsg.Attributes(&attributes_req);
    reqmsg.Context(&context);
    repmsg.Attributes(&attributes_rep);
    repmsg.Context(&context);

    Arc::MCC_Status status;
    std::cout << " Request sent. Waiting for the response." << std::endl;
    status= client_entry->process(reqmsg,repmsg);

    if(!status) {
      logger.msg(Arc::ERROR, "Request failed");
      std::cerr << "Status: " << std::string(status) << std::endl;
      return "-1";
    };

    Arc::PayloadSOAP* resp = NULL;
    if(repmsg.Payload() == NULL) {
      logger.msg(Arc::ERROR, "There is no response");
      return "-1";
    };
    try {
      resp = dynamic_cast<Arc::PayloadSOAP*>(repmsg.Payload());
    } catch(std::exception&) { };
    if(resp == NULL) {
      logger.msg(Arc::ERROR, "Response is not SOAP");
      return "-1";
    };

    //The response message
    std::string response = "";
    if (bool((*resp)["RegisterResponse"]) ){
          std::string xml;
          (*resp)["RegisterResponse"].GetDoc(xml, true);
          response += xml;
          response +="\n";
    }

    return response;
}


// RemoveRegistration function
std::string RemoveRegistration( Arc::URL url, std::vector<std::string> &serviceID ){

    Arc::XMLNode client_doc(ChainConfigString(url));
    Arc::Config client_config(client_doc);
    if(!client_config) {
      logger.msg(Arc::ERROR, "Failed to load client configuration");
      return "-1";
    };
    Arc::MCCLoader client_loader(client_config);
    logger.msg(Arc::INFO, "Client side MCCs are loaded");
    Arc::MCC* client_entry = client_loader["soap"];
    if(!client_entry) {
      logger.msg(Arc::ERROR, "Client chain does not have entry point");
      return "-1";
    };

    // Create and send RemoveRegistration request
    logger.msg(Arc::INFO, "Creating and sending request");
    Arc::NS query_ns; query_ns["isis"]="urn:isis";
    query_ns[""] = "http://www.nordugrid.org/schemas/isis/2007/06";
    Arc::PayloadSOAP req(query_ns);

    Arc::XMLNode request = req.NewChild("RemoveRegistrations");
    for (std::vector<std::string>::const_iterator it = serviceID.begin(); it != serviceID.end(); it++){
        request.NewChild("ServiceID") = *it;
    }
    Arc::Message reqmsg;
    Arc::Message repmsg;
    reqmsg.Payload(&req);
    // It is a responsibility of code initiating first Message to
    // provide Context and Attributes as well.
    Arc::MessageAttributes attributes_req;
    Arc::MessageAttributes attributes_rep;
    Arc::MessageContext context;
    reqmsg.Attributes(&attributes_req);
    reqmsg.Context(&context);
    repmsg.Attributes(&attributes_rep);
    repmsg.Context(&context);

    Arc::MCC_Status status;
    std::cout << " Request sent. Waiting for the response." << std::endl;
    status= client_entry->process(reqmsg,repmsg);

    if(!status) {
      logger.msg(Arc::ERROR, "Request failed");
      std::cerr << "Status: " << std::string(status) << std::endl;
      return "-1";
    };

    Arc::PayloadSOAP* resp = NULL;
    if(repmsg.Payload() == NULL) {
      logger.msg(Arc::ERROR, "There is no response");
      return "-1";
    };
    try {
      resp = dynamic_cast<Arc::PayloadSOAP*>(repmsg.Payload());
    } catch(std::exception&) { };
    if(resp == NULL) {
      logger.msg(Arc::ERROR, "Response is not SOAP");
      return "-1";
    };

    //The response message
    std::string response = "";
    if (bool((*resp)["RemoveRegistrationsResponse"]) ){
          std::string xml;
          (*resp)["RemoveRegistrationsResponse"].GetDoc(xml, true);
          response += xml;
          response +="\n";
    }

    return response;
}

// GetISISList function
std::vector<std::string> GetISISList( Arc::URL url ){

    //The response vector
    std::vector<std::string> response;

    Arc::XMLNode client_doc(ChainConfigString(url));
    Arc::Config client_config(client_doc);
    if(!client_config) {
      logger.msg(Arc::ERROR, "Failed to load client configuration");
      return response;
    };
    Arc::MCCLoader client_loader(client_config);
    logger.msg(Arc::INFO, "Client side MCCs are loaded");
    Arc::MCC* client_entry = client_loader["soap"];
    if(!client_entry) {
      logger.msg(Arc::ERROR, "Client chain does not have entry point");
      return response;
    };

    // Create and send RemoveRegistration request
    logger.msg(Arc::INFO, "Creating and sending request");
    Arc::NS query_ns; query_ns["isis"]="urn:isis";
    query_ns[""] = "http://www.nordugrid.org/schemas/isis/2007/06";
    Arc::PayloadSOAP req(query_ns);

    Arc::XMLNode request = req.NewChild("GetISISList");
    Arc::Message reqmsg;
    Arc::Message repmsg;
    reqmsg.Payload(&req);
    // It is a responsibility of code initiating first Message to
    // provide Context and Attributes as well.
    Arc::MessageAttributes attributes_req;
    Arc::MessageAttributes attributes_rep;
    Arc::MessageContext context;
    reqmsg.Attributes(&attributes_req);
    reqmsg.Context(&context);
    repmsg.Attributes(&attributes_rep);
    repmsg.Context(&context);

    Arc::MCC_Status status;
    std::cout << " Request sent. Waiting for the response." << std::endl;
    status= client_entry->process(reqmsg,repmsg);

    if(!status) {
      logger.msg(Arc::ERROR, "Request failed");
      std::cerr << "Status: " << std::string(status) << std::endl;
      return response;
    };

    Arc::PayloadSOAP* resp = NULL;
    if(repmsg.Payload() == NULL) {
      logger.msg(Arc::ERROR, "There is no response");
      return response;
    };
    try {
      resp = dynamic_cast<Arc::PayloadSOAP*>(repmsg.Payload());
    } catch(std::exception&) { };
    if(resp == NULL) {
      logger.msg(Arc::ERROR, "Response is not SOAP");
      return response;
    };

    // Construct the response vector
    if (bool((*resp)["GetISISListResponse"]) ){
        int i = 0;
        while((bool)(*resp)["GetISISListResponse"]["EPR"][i]) {
            response.push_back((std::string)(*resp)["GetISISListResponse"]["EPR"][i]);
            i++;
        }
    }

    return response;
}

// Split the given string by the given delimiter and return its parts
std::vector<std::string> split( const std::string original_string, const std::string delimiter ) {
    std::vector<std::string> retVal;
    unsigned long start=0;
    unsigned long end;
    while ( ( end = original_string.find( delimiter, start ) ) != std::string::npos ) {
          retVal.push_back( original_string.substr( start, end-start ) );
          start = end + 1;
    }
    retVal.push_back( original_string.substr( start ) );
    return retVal;
}


int main(int argc, char** argv) {

    Arc::LogStream logcerr(std::cerr);
    Arc::Logger::getRootLogger().addDestination(logcerr);
    Arc::Logger::getRootLogger().setThreshold(Arc::WARNING);

    Arc::OptionParser options(istring("[ISIS testing ...]"),
      istring("This tiny tool can be used for testing "
              "the ISIS's abilities."),
      istring("The method are the folows: Query, Register, RemoveRegistration")
      );

    std::string method = "";
      options.AddOption('m', "method",
        istring("define which method are use (Query, Register, RemoveRegistration)"),
        istring("method"),
        method);

    std::string debug;
      options.AddOption('d', "debug",
                istring("FATAL, ERROR, WARNING, INFO, DEBUG or VERBOSE"),
                istring("debuglevel"), debug); 

    std::list<std::string> parameters = options.Parse(argc, argv);
      if (parameters.empty()) {
         std::cout << "Use --help option for detailed usage information" << std::endl;
         return 1;
      }

    if (!debug.empty())
       Arc::Logger::getRootLogger().setThreshold(Arc::string_to_level(debug));

    std::cout << " [ ISIS tester ] " << std::endl;
    logger.msg(Arc::INFO, "ISIS tester start!");

    Arc::URL BootstrapISIS("http://knowarc2.grid.niif.hu:50000/isis1");

    // Get the a list of known ISIS's and choose one from them randomly
    std::vector<std::string> neighbors_ = GetISISList( BootstrapISIS );

    std::srand(time(NULL));
    std::string contactISIS_address_ = neighbors_[std::rand() % neighbors_.size()];
    std::cout << "The choosen ISIS list for contact detailed information: " << contactISIS_address_ << std::endl;
    Arc::URL ContactISIS(contactISIS_address_);
    // end of getISISList

    std::cout << " [ The selected method:  " << method << " ] " << std::endl;
    std::string response = "";
    //The method is Query
    if (method == "Query"){
       std::string query_string = "";
       for (std::list<std::string>::const_iterator it=parameters.begin(); it!=parameters.end(); it++){
           query_string += " " + *it;
       }
       response = Query( ContactISIS, *parameters.begin() );
       if ( response != "-1" ){
          Arc::XMLNode resp(response);
          Arc::XMLNode queryresponse_;
          resp["Body"]["QueryResponse"].New(queryresponse_);
          queryresponse_.GetDoc(response, true);
          std::cout << " The Query response: " << response << std::endl;
       }
    }
    //The method is Register
    else if (method == "Register"){
       std::vector<std::string> serviceID;
       std::vector<std::string> epr;

       for (std::list<std::string>::const_iterator it=parameters.begin(); it!=parameters.end(); it++){
           std::vector<std::string> Elements = split( *it, "," );
           if ( Elements.size() > 1) {
              serviceID.push_back(Elements[0]);
              epr.push_back(Elements[1]);
           }
           else {
              logger.msg(Arc::ERROR, "Not enough parameter! %s", *it);
              return 1;
           }
       }
       response = Register( ContactISIS, serviceID, epr );
       if ( response != "-1" ){
          Arc::XMLNode resp(response);
          if ( bool(resp["Body"]["Fault"]) ){ 
             std::cout << " The Registeration failed!" << std::endl;
             std::cout << " The fault's name: " << (std::string)resp["Body"]["Fault"]["Name"] << std::endl;
             std::cout << " The fault's type: " << (std::string)resp["Body"]["Fault"]["Type"] << std::endl;
             std::cout << " The fault's description: " << (std::string)resp["Body"]["Fault"]["Description"] << std::endl;
          }
          else {
             std::cout << " The Register method succeeded." << std::endl;
          }
       }
    }
    //The method is RemoveRegistration
    else if (method == "RemoveRegistration"){
       std::vector<std::string> serviceID;

       for (std::list<std::string>::const_iterator it=parameters.begin(); it!=parameters.end(); it++){
              serviceID.push_back(*it);
       }
       response = RemoveRegistration( ContactISIS, serviceID );
       if ( response != "-1" ){
          Arc::XMLNode resp(response);
          int i=0;
          Arc::XMLNode responsElements = resp["Body"]["RemoveRegistrationsResponse"]["RemoveRegistrationResponseElement"];
          while ( bool(responsElements[i]) ){
             std::cout << " The RemoveRegistration failed!" << std::endl;
             std::cout << " The ServiceID: " << (std::string)responsElements[i]["ServiceID"] << std::endl;
             std::cout << " The fault's name: " << (std::string)responsElements[i]["Fault"]["Name"] << std::endl;
             std::cout << " The fault's type: " << (std::string)responsElements[i]["Fault"]["Type"] << std::endl;
             std::cout << " The fault's description: " << (std::string)responsElements[i]["Fault"]["Description"] << std::endl;
             i++;
          }

          if (i == 0) {
             std::cout << " The RemoveRegistration method succeeded." << std::endl;
          }
       }
    }
    else {
       std::cout << " [ Wrong method! ] " << std::endl;
       return 0;
    }

    // When any problem is by the SOAP message.
    if ( response == "-1" ){
       std::cout << " No response message or other error in the " << method << " process!" << std::endl;
       return 1;
    }

    return 0;
}


