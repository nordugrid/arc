#include <string>
#include <list>
#include <arc/Logger.h>
#include <arc/UserConfig.h>
#include <arc/OptionParser.h>
#include <arc/Endpoint.h>
#include <arc/client/EndpointRetriever.h>

class TargetGenerator : public Arc::EndpointConsumer<Arc::ServiceEndpoint>, public Arc::EndpointContainer<Arc::ExecutionTarget> {
public:
  TargetGenerator(
    Arc::UserConfig uc,
    std::list<Arc::ServiceEndpoint> services,
    std::list<std::string> rejectedServices,
    std::list<std::string> preferredInterfaceNames = std::list<std::string>(),
    std::list<std::string> capabilityFilter = std::list<std::string>(1,Arc::ComputingInfoEndpoint::ComputingInfoCapability)
  );

  void wait();

  void addEndpoint(const Arc::ServiceEndpoint& service);

  void saveTargetInfoToStream(std::ostream& out, bool detailed);

private:
  Arc::ServiceEndpointRetriever ser;
  Arc::TargetInformationRetriever tir;
};


std::list<Arc::ServiceEndpoint> getServicesFromUserConfigAndCommandLine(Arc::UserConfig, std::list<std::string>, std::list<std::string>);

void showplugins(const std::string& program, const std::list<std::string>& types, Arc::Logger& logger, const std::string& chosenBroker = "");

bool checkproxy(const Arc::UserConfig& uc);

void splitendpoints(std::list<std::string>& selected, std::list<std::string>& rejected);

class ClientOptions : public Arc::OptionParser {
public:
  enum Client_t {
    CO_SUB, CO_MIGRATE, CO_RESUB, CO_TEST,
    CO_CAT, CO_CLEAN, CO_GET, CO_KILL, CO_RENEW, CO_RESUME, CO_STAT,
    CO_SYNC,
    CO_INFO,
    CO_ACL
 };

  ClientOptions(Client_t c,
                const std::string& arguments = "",
                const std::string& summary = "",
                const std::string& description = "");

  bool dryrun;
  bool dumpdescription;
  bool show_credentials;
  bool show_plugins;
  bool showversion;
  bool all;
  bool forcemigration;
  bool keep;
  bool forcesync;
  bool truncate;
  bool longlist;
  bool printids;
  bool same;
  bool notsame;
  bool forceclean;
  bool show_stdout;
  bool show_stderr;
  bool show_joblog;
  bool usejobname;
  bool forcedownload;

  int testjobid;
  int timeout;

  std::string joblist;
  std::string jobidoutfile;
  std::string conffile;
  std::string debug;
  std::string broker;
  std::string sort;
  std::string rsort;
  std::string downloaddir;

  std::list<std::string> clusters;
  std::list<std::string> qlusters;
  std::list<std::string> indexurls;
  std::list<std::string> jobdescriptionstrings;
  std::list<std::string> jobdescriptionfiles;
  std::list<std::string> jobidinfiles;
  std::list<std::string> status;
};
