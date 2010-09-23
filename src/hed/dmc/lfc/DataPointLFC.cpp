// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <cstdlib>
// NOTE: On Solaris errno is not working properly if cerrno is included first
#include <cerrno>

#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>

extern "C" {
/* See http://goc.grid.sinica.edu.tw/gocwiki/UsingLiblfcWithThreads
 * for LFC threading info */

/* use POSIX standard threads library */
#include <pthread.h>
extern int Cthread_init(); /* from <Cthread_api.h> */
#define thread_t      pthread_t
#define thread_create(tid_p, func, arg)  pthread_create(tid_p, NULL, func, arg)
#define thread_join(tid)               pthread_join(tid, NULL)

#include <lfc_api.h>

#ifndef _THREAD_SAFE
#define _THREAD_SAFE
#endif
#include <serrno.h>
}

#include <arc/GUID.h>
#include <arc/Logger.h>
#include <arc/StringConv.h>
#include <arc/URL.h>
#include <arc/Utils.h>

#include "DataPointLFC.h"

namespace Arc {

  Logger DataPointLFC::logger(DataPoint::logger, "LFC");

  DataPointLFC::DataPointLFC(const URL& url, const UserConfig& usercfg)
    : DataPointIndex(url, usercfg),
      guid("") {
    // set retry env variables (don't overwrite if set already)
    // connection timeout
    SetEnv("LFC_CONNTIMEOUT", "30", false);
    // number of retries
    SetEnv("LFC_CONRETRY", "1", false);
    // interval between retries
    SetEnv("LFC_CONRETRYINT", "10", false);

    // set host name env var
    SetEnv("LFC_HOST", url.Host());
  }

  DataPointLFC::~DataPointLFC() {}

  Plugin* DataPointLFC::Instance(PluginArgument *arg) {
    DataPointPluginArgument *dmcarg =
      dynamic_cast<DataPointPluginArgument*>(arg);
    if (!dmcarg)
      return NULL;
    if (((const URL&)(*dmcarg)).Protocol() != "lfc")
      return NULL;
    // Make this code non-unloadable because Globus
    // may have problems with unloading
    Glib::Module* module = dmcarg->get_module();
    PluginsFactory* factory = dmcarg->get_factory();
    if(factory && module) factory->makePersistent(module);
    return new DataPointLFC(*dmcarg, *dmcarg);
  }

  DataStatus DataPointLFC::Check() {
    // simply check that the file can be listed
    std::list<FileInfo> files;
    DataStatus res = ListFiles(files);
    if (!res.Passed() || files.size() == 0)
      return res;
    // set some metadata
    if (files.front().CheckSize())
      SetSize(files.front().GetSize());
    if (files.front().CheckCheckSum())
      SetCheckSum(files.front().GetCheckSum());
    if (files.front().CheckCreated())
      SetCreated(files.front().GetCreated());
    if (files.front().CheckValid())
      SetValid(files.front().GetValid());
    return DataStatus::Success;
  }

  /* perform resolve operation, which can take long time */
  DataStatus DataPointLFC::Resolve(bool source) {

#ifndef WITH_CTHREAD
    /* Initialize Cthread library - should be called before any LFC-API function */
    if (0 != Cthread_init()) {
      logger.msg(ERROR, "Cthread_init() error: %s", sstrerror(serrno));
      return DataStatus::SystemError;
    }
#endif

    if (lfc_startsess(const_cast<char*>(url.Host().c_str()),
                      const_cast<char*>("ARC")) != 0) {
      logger.msg(ERROR, "Error starting session: %s", sstrerror(serrno));
      lfc_endsess();
      if (serrno == SECOMERR || serrno == ENSNACT || serrno == SETIMEDOUT)
        return source ? DataStatus::ReadResolveErrorRetryable : DataStatus::WriteResolveErrorRetryable;
      return source ? DataStatus::ReadResolveError : DataStatus::WriteResolveError;
    }

    std::string path = url.Path();

    if (source || path.empty() || path == "/") {
      path = ResolveGUIDToLFN();
      if (path.empty()) {
        lfc_endsess();
        return source ? DataStatus::ReadResolveError : DataStatus::WriteResolveError;
      }
    }
    if (!source && url.Locations().size() == 0) {
      logger.msg(ERROR, "Locations are missing in destination LFC URL");
      lfc_endsess();
      return DataStatus::WriteResolveError;
    }

    resolved = false;
    registered = false;
    int nbentries = 0;
    struct lfc_filereplica *entries = NULL;
    if (lfc_getreplica(path.c_str(), NULL, NULL, &nbentries, &entries) != 0) {
      if (source || ((serrno != ENOENT) && (serrno != ENOTDIR))) {
        logger.msg(ERROR, "Error finding replicas: %s", sstrerror(serrno));
        lfc_endsess();
        return source ? DataStatus::ReadResolveError : DataStatus::WriteResolveError;
      }
      nbentries = 0;
      entries = NULL;
    }
    else
      registered = true;

    if (source) { // add locations resolved above
      for (int n = 0; n < nbentries; n++) {
        URL uloc(entries[n].sfn);
        if (!uloc) {
          logger.msg(WARNING, "Skipping invalid location: %s - %s", url.ConnectionURL(), entries[n].sfn);
          continue;
        }
        for (std::map<std::string, std::string>::const_iterator i = url.CommonLocOptions().begin();
             i != url.CommonLocOptions().end(); i++)
          uloc.AddOption(i->first, i->second, false);
        if (AddLocation(uloc, url.ConnectionURL()) == DataStatus::LocationAlreadyExistsError)
          logger.msg(WARNING, "Duplicate replica found in LFC: %s", uloc.plainstr());
        else
          logger.msg(VERBOSE, "Adding location: %s - %s", url.ConnectionURL(), entries[n].sfn);
      }
    }
    else { // add given locations, checking they don't already exist
      for (std::list<URLLocation>::const_iterator loc = url.Locations().begin(); loc != url.Locations().end(); ++loc) {

        // Make pfn from loc + last part of LFN 
        std::string pfn = loc->fullstr();
        if (pfn.find_last_of("/") != pfn.length() - 1)
          pfn += "/"; 
        // take off leading dirs of LFN
        std::string::size_type slash_index = path.rfind("/", path.length() + 1);
        if (slash_index != std::string::npos)
          pfn += path.substr(slash_index + 1);
        else
          pfn += path;

        URL uloc(pfn);
        if (!uloc) {
          logger.msg(WARNING, "Skipping invalid location: %s - %s", url.ConnectionURL(), pfn);
          continue;
        }

        // check that this replica doesn't exist already
        for (int n = 0; n < nbentries; n++) {
          if (std::string(entries[n].sfn) == uloc.plainstr()) {
            logger.msg(ERROR, "Replica %s already exists for LFN %s", entries[n].sfn, url.plainstr());
            return DataStatus::WriteResolveError;
          }
        }

        for (std::map<std::string, std::string>::const_iterator i = url.CommonLocOptions().begin();
             i != url.CommonLocOptions().end(); i++)
          uloc.AddOption(i->first, i->second, false);
        for (std::map<std::string, std::string>::const_iterator i = url.MetaDataOptions().begin();
             i != url.MetaDataOptions().end(); i++)
          uloc.AddMetaDataOption(i->first, i->second, false);

       if (AddLocation(uloc, url.ConnectionURL()) == DataStatus::LocationAlreadyExistsError)
          logger.msg(WARNING, "Duplicate replica location: %s", uloc.plainstr());
        else
          logger.msg(VERBOSE, "Adding location: %s - %s", url.ConnectionURL(), uloc.plainstr());
      }
    }
    if (entries)
      free(entries);

    struct lfc_filestatg st;
    if (lfc_statg(path.c_str(), NULL, &st) == 0) {
      if (st.filemode & S_IFDIR) { // directory
        lfc_endsess();
        return DataStatus::Success;
      }
      registered = true;
      SetSize(st.filesize);
      SetCreated(st.mtime);
      if (st.csumtype[0] && st.csumvalue[0]) {
        std::string csum = st.csumtype;
        if (csum == "MD")
          csum = "md5";
        else if (csum == "AD") 
          csum = "adler32";
        csum += ":";
        csum += st.csumvalue;
        SetCheckSum(csum);
      }
      guid = st.guid;
    }
    lfc_endsess();
    if (!HaveLocations()) {
      logger.msg(ERROR, "No locations found for %s", url.str());
      return source ? DataStatus::ReadResolveError : DataStatus::WriteResolveError;
    }
    if (CheckCheckSum()) logger.msg(VERBOSE, "meta_get_data: checksum: %s", GetCheckSum());
    if (CheckSize()) logger.msg(VERBOSE, "meta_get_data: size: %llu", GetSize());
    if (CheckCreated()) logger.msg(VERBOSE, "meta_get_data: created: %s", GetCreated().str());

    resolved = true;
    return DataStatus::Success;
  }

  DataStatus DataPointLFC::PreRegister(bool replication, bool force) {

#ifndef WITH_CTHREAD
    /* Initialize Cthread library - should be called before any LFC-API function */
    if (0 != Cthread_init()) {
      logger.msg(ERROR, "Cthread_init() error: %s", sstrerror(serrno));
      return DataStatus::SystemError;
    }
#endif

    if (replication) { /* replicating inside same lfn */
      if (!registered) {
        logger.msg(ERROR, "LFN is missing in LFC (needed for replication)");
        return DataStatus::PreRegisterError;
      }
      return DataStatus::Success;
    }
    if (registered) {
      if (!force) {
        logger.msg(ERROR, "LFN already exists in LFC");
        return DataStatus::PreRegisterError;
      }
      return DataStatus::Success;
    }
    if (lfc_startsess(const_cast<char*>(url.Host().c_str()),
                      const_cast<char*>("ARC")) != 0) {
      logger.msg(ERROR, "Error starting session: %s", sstrerror(serrno));
      if (serrno == SECOMERR || serrno == ENSNACT || serrno == SETIMEDOUT)
        return DataStatus::PreRegisterErrorRetryable;      
      return DataStatus::PreRegisterError;
    }
    if (!url.MetaDataOption("guid").empty()) {
      guid = url.MetaDataOption("guid");
      logger.msg(VERBOSE, "Using supplied guid %s", guid);
    }
    else if (guid.empty())
      guid = UUID();

    if (lfc_creatg(url.Path().c_str(), guid.c_str(),
                   S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP) != 0) {

      // check error - if it is no such file or directory, try to create the
      // required dirs
      if (serrno == ENOENT) {
        std::string::size_type slashpos = url.Path().find("/", 1); // don't create root dir
        while (slashpos != std::string::npos) {
          std::string dirname = url.Path().substr(0, slashpos);
          // list dir to see if it exists
          struct lfc_filestat statbuf;
          if (lfc_stat(dirname.c_str(), &statbuf) == 0) {
            slashpos = url.Path().find("/", slashpos + 1);
            continue;
          }

          logger.msg(VERBOSE, "Creating LFC directory %s", dirname);
          if (lfc_mkdir(dirname.c_str(), 0775) != 0)
            if (serrno != EEXIST) {
              logger.msg(ERROR, "Error creating required LFC dirs: %s", sstrerror(serrno));
              lfc_endsess();
              return DataStatus::PreRegisterError;
            }
          slashpos = url.Path().find("/", slashpos + 1);
        }
        if (lfc_creatg(url.Path().c_str(), guid.c_str(),
                       S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP) != 0 && serrno != EEXIST) {
          logger.msg(ERROR, "Error creating LFC entry: %s", sstrerror(serrno));
          lfc_endsess();
          return DataStatus::PreRegisterError;
        }
      }
      // LFN may already exist, caused by 2 uploaders simultaneously uploading to the same LFN
      // ignore the error if force is true and get the previously registered guid 
      else if (serrno == EEXIST && force) {
        struct lfc_filestatg st;
        if (lfc_statg(url.Path().c_str(), NULL, &st) == 0) {
          registered = true;
          guid = st.guid;
          lfc_endsess();
          return DataStatus::Success;
        }
        else {
          logger.msg(ERROR, "Error finding info on LFC entry %s which should exist: %s", url.Path(), sstrerror(serrno));
          lfc_endsess();
          return DataStatus::PreRegisterError;
        }
      }
      else {
        logger.msg(ERROR, "Error creating LFC entry %s, guid %s: %s", url.Path(), guid, sstrerror(serrno));
        lfc_endsess();
        return DataStatus::PreRegisterError;
      }
    }
    if (CheckCheckSum()) {
      std::string cksum = GetCheckSum();
      std::string::size_type p = cksum.find(':');
      if (p == std::string::npos) {
        std::string ckstype = cksum.substr(0,p);
        if (ckstype=="md5")
          ckstype = "MD";
        if (ckstype=="adler32")
          ckstype = "AD";
        // only md5 and adler32 are supported by LFC
        if (ckstype == "MD" || ckstype == "AD") {
          std::string cksumvalue = cksum.substr(p+1);
          if (CheckSize()) {
            if (lfc_setfsizeg(guid.c_str(), GetSize(), ckstype.c_str(), const_cast<char*>(cksumvalue.c_str())) != 0)
              logger.msg(ERROR, "Error entering metadata: %s", sstrerror(serrno));
          }
          else if (lfc_setfsizeg(guid.c_str(), 0, ckstype.c_str(), const_cast<char*>(cksumvalue.c_str())) != 0)
            logger.msg(ERROR, "Error entering metadata: %s", sstrerror(serrno));
        }
        else
          logger.msg(WARNING, "Warning: only md5 and adler32 checksums are supported by LFC");
      }
    }
    else if (CheckSize())
      if (lfc_setfsizeg(guid.c_str(), GetSize(), NULL, NULL) != 0)
        logger.msg(ERROR, "Error entering metadata: %s", sstrerror(serrno));

    lfc_endsess();
    return DataStatus::Success;
  }

  DataStatus DataPointLFC::PostRegister(bool replication) {

#ifndef WITH_CTHREAD
    /* Initialize Cthread library - should be called before any LFC-API function */
    if (0 != Cthread_init()) {
      logger.msg(ERROR, "Cthread_init() error: %s", sstrerror(serrno));
      return DataStatus::SystemError;
    }
#endif

    if (guid.empty()) {
      logger.msg(ERROR, "No GUID defined for LFN - probably not preregistered");
      return DataStatus::PostRegisterError;
    }
    if (lfc_startsess(const_cast<char*>(url.Host().c_str()), const_cast<char*>("ARC")) != 0) {
      logger.msg(ERROR, "Error starting session: %s", sstrerror(serrno));
      if (serrno == SECOMERR || serrno == ENSNACT || serrno == SETIMEDOUT)
        return DataStatus::PostRegisterErrorRetryable;
      return DataStatus::PostRegisterError;
    }
    if (lfc_addreplica(guid.c_str(), NULL, CurrentLocation().Host().c_str(), CurrentLocation().plainstr().c_str(), '-', 'P', NULL, NULL) != 0) {
      logger.msg(ERROR, "Error adding replica: %s", sstrerror(serrno));
      lfc_endsess();
      return DataStatus::PostRegisterError;
    }
    if (!replication && !registered) {
      if (CheckCheckSum()) {
        std::string cksum = GetCheckSum();
        std::string::size_type p = cksum.find(':');
        if(p != std::string::npos) {
          std::string ckstype = cksum.substr(0,p);
          if (ckstype=="md5")
            ckstype = "MD";
          if (ckstype=="adler32")
            ckstype = "AD";
          // only md5 and adler32 are supported by LFC
          if (ckstype == "MD" || ckstype == "AD") {
            std::string cksumvalue = cksum.substr(p+1);
            if (CheckSize()) {
              logger.msg(VERBOSE, "Entering checksum type %s, value %s, file size %llu", ckstype, cksumvalue, GetSize());
              if (lfc_setfsizeg(guid.c_str(), GetSize(), ckstype.c_str(), const_cast<char*>(cksumvalue.c_str())) != 0)
                logger.msg(ERROR, "Error entering metadata: %s", sstrerror(serrno));
            }
            else if (lfc_setfsizeg(guid.c_str(), 0, ckstype.c_str(), const_cast<char*>(cksumvalue.c_str())) != 0)
              logger.msg(ERROR, "Error entering metadata: %s", sstrerror(serrno));
          }
          else
            logger.msg(WARNING, "Warning: only md5 and adler32 checksums are supported by LFC");
        }
      }
      else if (CheckSize())
        if (lfc_setfsizeg(guid.c_str(), GetSize(), NULL, NULL) != 0)
          logger.msg(ERROR, "Error entering metadata: %s", sstrerror(serrno));
    }
    lfc_endsess();
    return DataStatus::Success;
  }

  DataStatus DataPointLFC::PreUnregister(bool replication) {

#ifndef WITH_CTHREAD
    /* Initialize Cthread library - should be called before any LFC-API function */
    if (0 != Cthread_init()) {
      logger.msg(ERROR, "Cthread_init() error: %s", sstrerror(serrno));
      return DataStatus::SystemError;
    }
#endif

    if (replication || registered)
      return DataStatus::Success;
    if (lfc_startsess(const_cast<char*>(url.Host().c_str()),
                      const_cast<char*>("ARC")) != 0) {
      logger.msg(ERROR, "Error starting session: %s", sstrerror(serrno));
      if (serrno == SECOMERR || serrno == ENSNACT || serrno == SETIMEDOUT)
        return DataStatus::UnregisterErrorRetryable;
      return DataStatus::UnregisterError;
    }
    std::string path = ResolveGUIDToLFN();
    if (path.empty()) {
      lfc_endsess();
      return DataStatus::UnregisterError;
    }
    if (lfc_unlink(path.c_str()) != 0)
      if ((serrno != ENOENT) && (serrno != ENOTDIR)) {
        logger.msg(ERROR, "Failed to remove LFN in LFC - You may need to do it by hand");
        lfc_endsess();
        return DataStatus::UnregisterError;
      }
    lfc_endsess();
    return DataStatus::Success;
  }

  DataStatus DataPointLFC::Unregister(bool all) {

#ifndef WITH_CTHREAD
    /* Initialize Cthread library - should be called before any LFC-API function */
    if (0 != Cthread_init()) {
      logger.msg(ERROR, "Cthread_init() error: %s", sstrerror(serrno));
      return DataStatus::SystemError;
    }
#endif

    if (!all && (!LocationValid())) {
      logger.msg(ERROR, "Location is missing");
      return DataStatus::UnregisterError;
    }
    if (lfc_startsess(const_cast<char*>(url.Host().c_str()),
                      const_cast<char*>("ARC")) != 0) {
      logger.msg(ERROR, "Error starting session: %s", sstrerror(serrno));
      if (serrno == SECOMERR || serrno == ENSNACT || serrno == SETIMEDOUT)
        return DataStatus::UnregisterErrorRetryable;
      return DataStatus::UnregisterError;
    }
    std::string path = ResolveGUIDToLFN();
    if (path.empty()) {
      lfc_endsess();
      return DataStatus::UnregisterError;
    }
    if (all) {
      int nbentries = 0;
      struct lfc_filereplica *entries = NULL;
      if (lfc_getreplica(path.c_str(), NULL, NULL, &nbentries, &entries) != 0) {
        lfc_endsess();
        if (serrno == ENOTDIR) {
          registered = false;
          ClearLocations();
          return DataStatus::Success;
        }
        logger.msg(ERROR, "Error getting replicas: %s", sstrerror(serrno));
        return DataStatus::UnregisterError;
      }
      for (int n = 0; n < nbentries; n++)
        if (lfc_delreplica(guid.c_str(), NULL, entries[n].sfn) != 0) {
          if (serrno == ENOENT)
            continue;
          lfc_endsess();
          logger.msg(ERROR, "Failed to remove location from LFC");
          return DataStatus::UnregisterError;
        }
      if (lfc_unlink(path.c_str()) != 0) {
        if (serrno == EPERM) { // we have a directory
          if (lfc_rmdir(path.c_str()) != 0) {
            if (serrno == EEXIST) { // still files in the directory
              logger.msg(ERROR, "Failed to remove LFC directory: directory is not empty");
              lfc_endsess();
              return DataStatus::UnregisterError;
            }
            logger.msg(ERROR, "Failed to remove LFC directory: %s", sstrerror(serrno));
            lfc_endsess();
            return DataStatus::UnregisterError;
          }
        }
        else if ((serrno != ENOENT) && (serrno != ENOTDIR)) {
          logger.msg(ERROR, "Failed to remove LFN in LFC: %s", sstrerror(serrno));
          lfc_endsess();
          return DataStatus::UnregisterError;
        }
      }
      registered = false;
    }
    else if (lfc_delreplica(guid.c_str(), NULL, CurrentLocation().plainstr().c_str()) != 0) {
      lfc_endsess();
      logger.msg(ERROR, "Failed to remove location from LFC: %s", sstrerror(serrno));
      return DataStatus::UnregisterError;
    }
    lfc_endsess();
    return DataStatus::Success;
  }


  DataStatus DataPointLFC::ListFiles(std::list<FileInfo>& files,
                                     bool long_list,
                                     bool resolve,
                                     bool metadata) {

#ifndef WITH_CTHREAD
    /* Initialize Cthread library - should be called before any LFC-API function */
    if (0 != Cthread_init()) {
      logger.msg(ERROR, "Cthread_init() error: %s", sstrerror(serrno));
      return DataStatus::SystemError;
    }
#endif

    if (lfc_startsess(const_cast<char*>(url.Host().c_str()),
                      const_cast<char*>("ARC")) != 0) {
      logger.msg(ERROR, "Error starting session: %s", sstrerror(serrno));
      if (serrno == SECOMERR || serrno == ENSNACT || serrno == SETIMEDOUT)
        return DataStatus::ListErrorRetryable;
      return DataStatus::ListError;
    }
    std::string path = ResolveGUIDToLFN();
    if (path.empty()) {
      lfc_endsess();
      return DataStatus::ListError;
    }
    // first stat the url and see if it is a file or directory
    struct lfc_filestatg st;

    if (lfc_statg(path.c_str(), NULL, &st) != 0) {
      logger.msg(ERROR, "Error listing file or directory: %s", sstrerror(serrno));
      lfc_endsess();
      return DataStatus::ListError;
    }

    // if it's a directory, list entries
    if (st.filemode & S_IFDIR && !metadata) {
      lfc_DIR *dir = lfc_opendirxg(const_cast<char*>(url.Host().c_str()), path.c_str(), NULL);
      if (dir == NULL) {
        logger.msg(ERROR, "Error opening directory: %s", sstrerror(serrno));
        lfc_endsess();
        return DataStatus::ListError;
      }

      // list entries. If not long list use readdir
      // does funny things with pointers, so always use long listing for now
      if (false) { // if (!long_list) {
        struct dirent *direntry;
        while ((direntry = lfc_readdir(dir)))
          std::list<FileInfo>::iterator f = files.insert(files.end(), FileInfo(direntry->d_name));
      }
      else { // for long list use readdirg
        struct lfc_direnstatg *direntry;
        while ((direntry = lfc_readdirg(dir))) {
          std::list<FileInfo>::iterator f = files.insert(files.end(), FileInfo(direntry->d_name));

          f->SetSize(direntry->filesize);
          if (strcmp(direntry->csumtype, "") != 0 && strcmp(direntry->csumvalue, "") != 0) {
            std::string csum = direntry->csumtype;
            csum += ":";
            csum += direntry->csumvalue;
            f->SetCheckSum(csum);
          }
          f->SetCreated(direntry->ctime);
          f->SetType((direntry->filemode & S_IFDIR) ? FileInfo::file_type_dir : FileInfo::file_type_file);
        }
      }
      if (serrno) {
        logger.msg(ERROR, "Error listing directory: %s", sstrerror(serrno));
        lfc_closedir(dir);
        lfc_endsess();
        return DataStatus::ListError;
      }

      // if we want to resolve replicas call readdirxr
      if (resolve) {
        lfc_rewinddir(dir);
        struct lfc_direnrep *direntry;

        while ((direntry = lfc_readdirxr(dir, NULL)))
          for (std::list<FileInfo>::iterator f = files.begin();
               f != files.end();
               f++)
            if (f->GetName() == direntry->d_name) {
              for (int n = 0; n < direntry->nbreplicas; n++)
                f->AddURL(URL(std::string(direntry->rep[n].sfn)));
              break;
            }
        if (serrno) {
          logger.msg(ERROR, "Error listing directory: %s", sstrerror(serrno));
          lfc_closedir(dir);
          lfc_endsess();
          return DataStatus::ListError;
        }
      }
      lfc_closedir(dir);
    } // if (dir)
    else {
      std::list<FileInfo>::iterator f = files.insert(files.end(), FileInfo(path.c_str()));
      f->SetMetaData("path", path);
      f->SetSize(st.filesize);
      f->SetMetaData("size", tostring(st.filesize));
      if (st.csumvalue[0]) {
        std::string csum = st.csumtype;
        csum += ":";
        csum += st.csumvalue;
        f->SetCheckSum(csum);
        f->SetMetaData("checksum", csum);
      }
      f->SetCreated(st.mtime);
      f->SetMetaData("mtime", f->GetCreated().str());
      f->SetType((st.filemode & S_IFDIR) ? FileInfo::file_type_dir : FileInfo::file_type_file);
      f->SetMetaData("type", (st.filemode & S_IFDIR) ? "dir" : "file");
      if (resolve) {
        int nbentries = 0;
        struct lfc_filereplica *entries = NULL;

        if (lfc_getreplica(path.c_str(), NULL, NULL, &nbentries, &entries) != 0) {
          logger.msg(ERROR, "Error listing replicas: %s", sstrerror(serrno));
          lfc_endsess();
          return DataStatus::ListError;
        }
        for (int n = 0; n < nbentries; n++)
          f->AddURL(URL(std::string(entries[n].sfn)));
      }
      // fill some more metadata
      if (st.guid[0] != '\0') f->SetMetaData("guid", std::string(st.guid));
      if(metadata) {
        char username[256];
        if (lfc_getusrbyuid(st.uid, username) == 0) f->SetMetaData("owner", username);
        char groupname[256];
        if (lfc_getgrpbygid(st.gid, groupname) == 0) f->SetMetaData("group", groupname);
      };
      mode_t mode = st.filemode;
      std::string perms;
      if (mode & S_IRUSR) perms += 'r'; else perms += '-';
      if (mode & S_IWUSR) perms += 'w'; else perms += '-';
      if (mode & S_IXUSR) perms += 'x'; else perms += '-';
      if (mode & S_IRGRP) perms += 'r'; else perms += '-';
      if (mode & S_IWGRP) perms += 'w'; else perms += '-';
      if (mode & S_IXGRP) perms += 'x'; else perms += '-';
      if (mode & S_IROTH) perms += 'r'; else perms += '-';
      if (mode & S_IWOTH) perms += 'w'; else perms += '-';
      if (mode & S_IXOTH) perms += 'x'; else perms += '-';
      f->SetMetaData("accessperm", perms);
      f->SetMetaData("ctime", (Time(st.ctime)).str());
      f->SetMetaData("atime", (Time(st.atime)).str());
    }
    lfc_endsess();
    return DataStatus::Success;
  }

  std::string DataPointLFC::ResolveGUIDToLFN() {

    // check if guid is already defined
    if (!guid.empty())
      return url.Path();

    // check for guid in the attributes
    if (url.MetaDataOption("guid").empty())
      return url.Path();

    guid = url.MetaDataOption("guid");

    lfc_list listp;
    struct lfc_linkinfo *info = lfc_listlinks(NULL, (char*)guid.c_str(),
                                              CNS_LIST_BEGIN, &listp);
    if (!info) {
      logger.msg(ERROR, "Error finding LFN from guid %s: %s",
                 guid, sstrerror(serrno));
      return "";
    }

    logger.msg(VERBOSE, "guid %s resolved to LFN %s", guid, info[0].path);
    std::string path = info[0].path;
    lfc_listlinks(NULL, (char*)guid.c_str(), CNS_LIST_END, &listp);
    return path;
  }

  const std::string DataPointLFC::DefaultCheckSum() const {
    return std::string("adler32");
  }

  std::string DataPointLFC::str() const {
    std::string tmp = url.plainstr(); // url with no options or meta-options
    // add guid if supplied
    if (!url.MetaDataOption("guid").empty())
      tmp += ":guid=" + url.MetaDataOption("guid");
    return tmp;
  }

} // namespace Arc

Arc::PluginDescriptor PLUGINS_TABLE_NAME[] = {
  { "lfc", "HED:DMC", 0, &Arc::DataPointLFC::Instance },
  { NULL, NULL, 0, NULL }
};
