// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/ArcConfig.h>
#include <arc/FileLock.h>
#include <arc/StringConv.h>
#include <arc/Thread.h>
#include <arc/compute/ExecutionTarget.h>
#include <arc/compute/Job.h>
#include <arc/compute/JobDescription.h>
#include <arc/compute/SubmitterPlugin.h>
#include <arc/UserConfig.h>
#include <arc/data/FileCache.h>
#include <arc/CheckSum.h>
#include <arc/data/DataMover.h>
#include <arc/data/URLMap.h>
#include <arc/loader/FinderLoader.h>


namespace Arc {

  Logger SubmitterPlugin::logger(Logger::getRootLogger(), "SubmitterPlugin");

  std::map<std::string, std::string> SubmitterPluginLoader::interfacePluginMap;

  SubmissionStatus SubmitterPlugin::Submit(const std::list<JobDescription>& jobdescs,
                                           const std::string& endpoint,
                                           EntityConsumer<Job>& jc,
                                           std::list<const JobDescription*>& notSubmitted) {
    for (std::list<JobDescription>::const_iterator it = jobdescs.begin();
         it != jobdescs.end(); ++it) {
      notSubmitted.push_back(&*it);
    }
    return SubmissionStatus::NOT_IMPLEMENTED | SubmissionStatus::DESCRIPTION_NOT_SUBMITTED;
  }

  void SubmitterPlugin::SetUserConfig(const UserConfig& uc) {
    // Changing user configuration may change identity.
    // Hence all open connections become invalid.
    if(!usercfg || !usercfg->IsSameIdentity(uc)) {
      delete dest_handle;
      dest_handle = NULL;
    }
    usercfg = &uc;
  }

  bool SubmitterPlugin::PutFiles(const JobDescription& job, const URL& url) const {
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
          dst.AddOption("blocksize=1048576",false);
          dst.AddOption("checksum=no",false);
          DataHandle source(src, *usercfg);
	  if (!source) {
            logger.msg(ERROR, "Failed reading file %s", src.fullstr());
            return false;
	  }
          if ((!dest_handle) || (!*dest_handle) || (!(*dest_handle)->SetURL(dst))) {
            if(dest_handle) delete dest_handle;
            ((SubmitterPlugin*)this)->dest_handle = new DataHandle(dst, *usercfg);
          };
          DataHandle& destination = *dest_handle;
          source->SetTries((src.Protocol() == "file")?1:3);
          destination->SetTries((dst.Protocol() == "file")?1:3);
          DataStatus res =
            mover.Transfer(*source, *destination, cache, URLMap(), 0, 0, 0,
                           usercfg->Timeout());
          if (!res.Passed()) {
            logger.msg(ERROR, "Failed uploading file %s to %s: %s", source->GetURL().fullstr(), destination->GetURL().fullstr(), std::string(res));
            return false;
          }
        }
      }

    return true;
  }

  void SubmitterPlugin::AddJobDetails(const JobDescription& jobdesc, Job& job) const {
    if (!jobdesc.Identification.JobName.empty()) {
      job.Name = jobdesc.Identification.JobName;
    }
    job.LocalSubmissionTime = Arc::Time().str(UTCTime);

    job.ActivityOldID = jobdesc.Identification.ActivityOldID;

    jobdesc.UnParse(job.JobDescriptionDocument, !jobdesc.GetSourceLanguage().empty() ? jobdesc.GetSourceLanguage() : "nordugrid:xrsl"); // Assuming job description is valid.

    job.LocalInputFiles.clear();
    for (std::list<InputFileType>::const_iterator it = jobdesc.DataStaging.InputFiles.begin();
         it != jobdesc.DataStaging.InputFiles.end(); it++) {
      if (!it->Sources.empty() && it->Sources.front().Protocol() == "file") {
        job.LocalInputFiles[it->Name] = it->Checksum; //CheckSumAny::FileChecksum(it->Sources.front().Path(), CheckSumAny::cksum, true);
      }
    }
    job.StdIn = jobdesc.Application.Input;
    job.StdOut = jobdesc.Application.Output;
    job.StdErr = jobdesc.Application.Error;
  }

  SubmitterPluginLoader::SubmitterPluginLoader() : Loader(BaseConfig().MakeConfig(Config()).Parent()) {}

  SubmitterPluginLoader::~SubmitterPluginLoader() {
    for (std::multimap<std::string, SubmitterPlugin*>::iterator it = submitters.begin(); it != submitters.end(); ++it) {
      delete it->second;
    }
  }

  void SubmitterPluginLoader::initialiseInterfacePluginMap(const UserConfig& uc) {
    if (!interfacePluginMap.empty()) {
      return;
    }

    std::list<ModuleDesc> modules;
    PluginsFactory factory(BaseConfig().MakeConfig(Config()).Parent());
    factory.scan(FinderLoader::GetLibrariesList(), modules);
    PluginsFactory::FilterByKind("HED:SubmitterPlugin", modules);
    std::list<std::string> availablePlugins;
    for (std::list<ModuleDesc>::const_iterator it = modules.begin(); it != modules.end(); ++it) {
      for (std::list<PluginDesc>::const_iterator it2 = it->plugins.begin(); it2 != it->plugins.end(); ++it2) {
        availablePlugins.push_back(it2->name);
      }
    }

    if (interfacePluginMap.empty()) {
      // Map supported interfaces to available plugins.
      for (std::list<std::string>::iterator itT = availablePlugins.begin(); itT != availablePlugins.end(); ++itT) {
        SubmitterPlugin* p = load(*itT, uc);

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

  SubmitterPlugin* SubmitterPluginLoader::load(const std::string& name,
                                               const UserConfig& usercfg) {
    if (name.empty())
      return NULL;

    if(!factory_->load(FinderLoader::GetLibrariesList(),
                       "HED:SubmitterPlugin", name)) {
      logger.msg(ERROR, "Unable to locate the \"%s\" plugin. Please refer to installation instructions and check if package providing support for \"%s\" plugin is installed", name, name);
      logger.msg(DEBUG, "SubmitterPlugin plugin \"%s\" not found.", name);
      return NULL;
    }

    SubmitterPluginArgument arg(usercfg);
    SubmitterPlugin *submitter =
      factory_->GetInstance<SubmitterPlugin>("HED:SubmitterPlugin", name, &arg, false);

    if (!submitter) {
      logger.msg(ERROR, "Unable to locate the \"%s\" plugin. Please refer to installation instructions and check if package providing support for \"%s\" plugin is installed", name, name);
      logger.msg(DEBUG, "SubmitterPlugin %s could not be created", name);
      return NULL;
    }

    submitters.insert(std::pair<std::string, SubmitterPlugin*>(name, submitter));
    logger.msg(DEBUG, "Loaded SubmitterPlugin %s", name);
    return submitter;
  }

  SubmitterPlugin* SubmitterPluginLoader::loadByInterfaceName(const std::string& name, const UserConfig& uc) {
    if (interfacePluginMap.empty()) {
      initialiseInterfacePluginMap(uc);
    }

    std::map<std::string, std::string>::const_iterator itPN = interfacePluginMap.find(name);
    if (itPN != interfacePluginMap.end()) {
      std::map<std::string, SubmitterPlugin*>::iterator itS = submitters.find(itPN->second);
      if (itS != submitters.end()) {
        itS->second->SetUserConfig(uc);
        return itS->second;
      }

      return load(itPN->second, uc);
    }

    return NULL;
  }

} // namespace Arc
