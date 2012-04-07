// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/ArcConfig.h>
#include <arc/FileLock.h>
#include <arc/StringConv.h>
#include <arc/Thread.h>
#include <arc/client/ExecutionTarget.h>
#include <arc/client/Job.h>
#include <arc/client/JobDescription.h>
#include <arc/client/Submitter.h>
#include <arc/UserConfig.h>
#include <arc/data/FileCache.h>
#include <arc/CheckSum.h>
#include <arc/data/DataMover.h>
#include <arc/data/DataHandle.h>
#include <arc/data/URLMap.h>
#include <arc/loader/FinderLoader.h>


namespace Arc {

  Logger Submitter::logger(Logger::getRootLogger(), "Submitter");

  std::map<std::string, std::string> SubmitterLoader::interfacePluginMap;

  bool Submitter::PutFiles(const JobDescription& job, const URL& url) const {
    FileCache cache;
    DataMover mover;
    mover.retry(true);
    mover.secure(false);
    mover.passive(true);
    mover.verbose(false);

    for (std::list<InputFileType>::const_iterator it = job.DataStaging.InputFiles.begin();
         it != job.DataStaging.InputFiles.end(); ++it)
      if (!it->Sources.empty()) {
        const URL& src = it->Sources.front();
        if (src.Protocol() == "file") {
          if(!url) {
            logger.msg(ERROR, "No stagein URL is provided");
            return false;
          };
          URL dst(url);
          dst.ChangePath(dst.Path() + '/' + it->Name);
          DataHandle source(src, usercfg);
          DataHandle destination(dst, usercfg);
          DataStatus res =
            mover.Transfer(*source, *destination, cache, URLMap(), 0, 0, 0,
                           usercfg.Timeout());
          if (!res.Passed()) {
            if (!res.GetDesc().empty()) {
              logger.msg(ERROR, "Failed uploading file: %s - %s",
                         std::string(res), res.GetDesc());
            } else {
              logger.msg(ERROR, "Failed uploading file: %s", std::string(res));
            }
            return false;
          }
        }
      }

    return true;
  }

  void Submitter::AddJobDetails(const JobDescription& jobdesc, const URL& jobid,
                                const URL& cluster, const URL& infoendpoint,
                                Job& job) const {
    job.JobID = jobid;
    if (!jobdesc.Identification.JobName.empty()) {
      job.Name = jobdesc.Identification.JobName;
    }
    if (!supportedInterfaces.empty()) {
      job.InterfaceName = supportedInterfaces.front();
    }
    job.Cluster = cluster;
    job.InfoEndpoint = infoendpoint;
    job.LocalSubmissionTime = Arc::Time().str(UTCTime);

    job.ActivityOldID = jobdesc.Identification.ActivityOldID;

    jobdesc.UnParse(job.JobDescriptionDocument, !jobdesc.GetSourceLanguage().empty() ? jobdesc.GetSourceLanguage() : "nordugrid:jsdl"); // Assuming job description is valid.

    job.LocalInputFiles.clear();
    for (std::list<InputFileType>::const_iterator it = jobdesc.DataStaging.InputFiles.begin();
         it != jobdesc.DataStaging.InputFiles.end(); it++) {
      if (!it->Sources.empty() && it->Sources.front().Protocol() == "file") {
        job.LocalInputFiles[it->Name] = CheckSumAny::FileChecksum(it->Sources.front().Path(), CheckSumAny::cksum, true);
      }
    }
  }

  SubmitterLoader::SubmitterLoader() : Loader(BaseConfig().MakeConfig(Config()).Parent()) {}

  SubmitterLoader::~SubmitterLoader() {
    for (std::multimap<std::string, Submitter*>::iterator it = submitters.begin(); it != submitters.end(); ++it)
      delete it->second;
  }

  void SubmitterLoader::initialiseInterfacePluginMap(const UserConfig& uc) {
    std::list<ModuleDesc> modules;
    PluginsFactory factory(BaseConfig().MakeConfig(Config()).Parent());
    factory.scan(FinderLoader::GetLibrariesList(), modules);
    PluginsFactory::FilterByKind("HED:Submitter", modules);
    std::list<std::string> availablePlugins;
    for (std::list<ModuleDesc>::const_iterator it = modules.begin(); it != modules.end(); ++it) {
      for (std::list<PluginDesc>::const_iterator it2 = it->plugins.begin(); it2 != it->plugins.end(); ++it2) {
        availablePlugins.push_back(it2->name);
      }
    }

    if (interfacePluginMap.empty()) {
      // Map supported interfaces to available plugins.
      for (std::list<std::string>::iterator itT = availablePlugins.begin(); itT != availablePlugins.end(); ++itT) {
        Submitter* p = load(*itT, uc);
  
        if (!p) {
          continue;
        }
  
        for (std::list<std::string>::const_iterator itI = p->SupportedInterfaces().begin(); itI != p->SupportedInterfaces().end(); ++itI) {
          if (!itT->empty()) { // Do not allow empty interface.
            // If two plugins supports two identical interfaces, then only the last will appear in the map.
            interfacePluginMap[*itI] = *itT;
          }
        }
      }
    }
  }

  Submitter* SubmitterLoader::load(const std::string& name,
                                   const UserConfig& usercfg) {
    if (name.empty())
      return NULL;

    if(!factory_->load(FinderLoader::GetLibrariesList(),
                       "HED:Submitter", name)) {
      logger.msg(ERROR, "Unable to locate the \"%s\" plugin. Please refer to installation instructions and check if package providing support for \"%s\" plugin is installed", name, name);
      logger.msg(DEBUG, "Submitter plugin \"%s\" not found.", name);
      return NULL;
    }

    SubmitterPluginArgument arg(usercfg);
    Submitter *submitter =
      factory_->GetInstance<Submitter>("HED:Submitter", name, &arg, false);

    if (!submitter) {
      logger.msg(ERROR, "Unable to locate the \"%s\" plugin. Please refer to installation instructions and check if package providing support for \"%s\" plugin is installed", name, name);
      logger.msg(DEBUG, "Submitter %s could not be created", name);
      return NULL;
    }

    submitters.insert(std::pair<std::string, Submitter*>(name, submitter));
    logger.msg(DEBUG, "Loaded Submitter %s", name);
    return submitter;
  }

  Submitter* SubmitterLoader::loadByInterfaceName(const std::string& name, const UserConfig& uc) {
    if (interfacePluginMap.empty()) {
      initialiseInterfacePluginMap(uc);
    }
    
    std::map<std::string, std::string>::const_iterator itPN = interfacePluginMap.find(name);
    if (itPN != interfacePluginMap.end()) {
      std::map<std::string, Submitter*>::iterator itS = submitters.find(itPN->second);
      if (itS != submitters.end()) {
        return itS->second;
      }
      
      return load(itPN->second, uc);
    }

    return NULL;
  }
} // namespace Arc
