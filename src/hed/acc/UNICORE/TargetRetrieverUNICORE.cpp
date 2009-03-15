// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/ArcConfig.h>
#include <arc/message/MCC.h>
#include <arc/Logger.h>
#include <arc/StringConv.h>
#include <arc/Thread.h>
#include <arc/URL.h>
#include <arc/XMLNode.h>
#include <arc/client/ExecutionTarget.h>
#include <arc/client/TargetGenerator.h>

#include <iostream>

#include "UNICOREClient.h"
#include "TargetRetrieverUNICORE.h"

namespace Arc {

  struct ThreadArg {
    TargetGenerator *mom;
    std::string proxyPath;
    std::string certificatePath;
    std::string keyPath;
    std::string caCertificatesDir;
    URL url;
    int targetType;
    int detailLevel;
  };

  ThreadArg* TargetRetrieverUNICORE::CreateThreadArg(TargetGenerator& mom,
                                                     int targetType,
                                                     int detailLevel) {
    ThreadArg *arg = new ThreadArg;
    arg->mom = &mom;
    arg->proxyPath = proxyPath;
    arg->certificatePath = certificatePath;
    arg->keyPath = keyPath;
    arg->caCertificatesDir = caCertificatesDir;
    arg->url = url;
    arg->targetType = targetType;
    arg->detailLevel = detailLevel;
    return arg;
  }

  Logger TargetRetrieverUNICORE::logger(TargetRetriever::logger, "UNICORE");

  TargetRetrieverUNICORE::TargetRetrieverUNICORE(Config *cfg)
    : TargetRetriever(cfg, "UNICORE") {}

  TargetRetrieverUNICORE::~TargetRetrieverUNICORE() {}

  Plugin* TargetRetrieverUNICORE::Instance(PluginArgument *arg) {
    ACCPluginArgument *accarg =
      arg ? dynamic_cast<ACCPluginArgument*>(arg) : NULL;
    if (!accarg)
      return NULL;
    return new TargetRetrieverUNICORE((Arc::Config*)(*accarg));
  }

  void TargetRetrieverUNICORE::GetTargets(TargetGenerator& mom, int targetType,
                                          int detailLevel) {

    logger.msg(INFO, "TargetRetriverUNICORE initialized with %s service url: %s",
               serviceType, url.str());

    if (serviceType == "computing") {
      bool added = mom.AddService(url);
      if (added) {
        ThreadArg *arg = CreateThreadArg(mom, targetType, detailLevel);
        if (!CreateThreadFunction(&InterrogateTarget, arg)) {
          delete arg;
          mom.RetrieverDone();
        }
      }
    }
    else if (serviceType == "storage") {}
    else if (serviceType == "index") {
      bool added = mom.AddIndexServer(url);
      if (added) {
        ThreadArg *arg = CreateThreadArg(mom, targetType, detailLevel);
        if (!CreateThreadFunction(&QueryIndex, arg)) {
          delete arg;
          mom.RetrieverDone();
        }
      }
    }
    else
      logger.msg(ERROR,
                 "TargetRetrieverUNICORE initialized with unknown url type");
  }

  void TargetRetrieverUNICORE::QueryIndex(void *arg) {
    ThreadArg *thrarg = (ThreadArg*)arg;
    TargetGenerator& mom = *thrarg->mom;

    URL& url = thrarg->url;
    MCCConfig cfg;
    /*    if (!thrarg->proxyPath.empty())
          cfg.AddProxy(thrarg->proxyPath);*/                                           //Normally proxies should not be used, possibly some provisions should be made if the user insists.
    if (!thrarg->certificatePath.empty())
      cfg.AddCertificate(thrarg->certificatePath);
    if (!thrarg->keyPath.empty())
      cfg.AddPrivateKey(thrarg->keyPath);
    if (!thrarg->caCertificatesDir.empty())
      cfg.AddCADir(thrarg->caCertificatesDir);
    std::cout << "Cert: " << thrarg->certificatePath << "  Key: " << thrarg->keyPath << std::endl;

    UNICOREClient uc(url, cfg);
    std::string thePayload;
    std::list<Arc::Config> beses;
    //beses should hold a list of trees each suitable to configure a new TargetRetriever
    uc.listTargetSystemFactories(beses, thePayload);
    std::cout << thePayload << std::endl; //debug remove!
    //The following loop should work even for mixed lists of index and computing services
    for (std::list<Arc::Config>::iterator it = beses.begin(); it != beses.end(); it++) {
      if (!thrarg->certificatePath.empty())
        (*it).NewChild("CertificatePath") = thrarg->certificatePath;
      if (!thrarg->keyPath.empty())
        (*it).NewChild("KeyPath") = thrarg->keyPath;
      if (!thrarg->caCertificatesDir.empty())
        (*it).NewChild("CACertificatesDir") = thrarg->caCertificatesDir;
      if (!thrarg->proxyPath.empty())
        (*it).NewChild("ProxyPath") = thrarg->proxyPath;
      TargetRetrieverUNICORE r(&(*it));
      r.GetTargets(mom, thrarg->targetType, thrarg->detailLevel);
    }

    delete thrarg;
    mom.RetrieverDone();
  }

  void TargetRetrieverUNICORE::InterrogateTarget(void *arg) {
    ThreadArg *thrarg = (ThreadArg*)arg;
    TargetGenerator& mom = *thrarg->mom;

    URL& url = thrarg->url;
    MCCConfig cfg;
    /*    if (!thrarg->proxyPath.empty())
               cfg.AddProxy(thrarg->proxyPath);*/                                           //Normally proxies should not be used, possibly some provisions should be made if the user insists.
    if (!thrarg->certificatePath.empty())
      cfg.AddCertificate(thrarg->certificatePath);
    if (!thrarg->keyPath.empty())
      cfg.AddPrivateKey(thrarg->keyPath);
    if (!thrarg->caCertificatesDir.empty())
      cfg.AddCADir(thrarg->caCertificatesDir);
    UNICOREClient uc(url, cfg);
    std::string status;
    if (!uc.sstat(status)) {
      delete thrarg;
      mom.RetrieverDone();
      return;
    }
    std::cout << status << std::endl; //debug remove!


    ExecutionTarget target;

    target.GridFlavour = "UNICORE";
    target.Cluster = url;
    target.url = url;
    target.InterfaceName = "BES";
    target.Implementor = "Unicore";
    target.ImplementationName = "Unicore";
    target.HealthState = "ok";

    target.DomainName = url.Host();


    delete thrarg;
    mom.AddTarget(target);
    mom.RetrieverDone();
  }

} // namespace Arc
