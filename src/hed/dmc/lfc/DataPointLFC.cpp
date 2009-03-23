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

  DataPointLFC::DataPointLFC(const URL& url)
    : DataPointIndex(url),
      guid("") {
    // set retry env variables (don't overwrite if set already)
    // connection timeout
    SetEnv("LFC_CONNTIMEOUT", "30");
    // number of retries
    SetEnv("LFC_CONRETRY", "1");
    // interval between retries
    SetEnv("LFC_CONRETRYINT", "10");

    // set host name env var
    SetEnv("LFC_HOST", url.Host());
  }

  DataPointLFC::~DataPointLFC() {}

  /* perform resolve operation, which can take long time */
  DataStatus DataPointLFC::Resolve(bool source) {

#ifndef WITH_CTHREAD
    /* Initialize Cthread library - should be called before any LFC-API function */
    if (0 != Cthread_init()) {
      logger.msg(ERROR, "Cthread_init() error: %s", sstrerror(serrno));
      return DataStatus::ReadResolveError;
    }
#endif

    if (lfc_startsess(const_cast<char*>(url.Host().c_str()),
                      const_cast<char*>("ARC")) != 0) {
      logger.msg(ERROR, "Error starting session: %s", sstrerror(serrno));
      lfc_endsess();
      return DataStatus::ReadResolveError;
    }

    if (source && !resolveGUIDToLFN()) {
      lfc_endsess();
      return DataStatus::ReadResolveError;
    }
    if (!source && !url.MetaDataOption("guid").empty()) {
      guid = url.MetaDataOption("guid");
      logger.msg(DEBUG, "Using supplied guid %s", guid);
    }

    resolved = false;
    registered = false;
    if (source) {
      if (url.Path().empty()) {
        logger.msg(INFO, "Source must contain LFN");
        return DataStatus::ReadResolveError;
      }
    }
    else {
      if (url.Path().empty()) {
        logger.msg(INFO, "Destination must contain LFN");
        return DataStatus::WriteResolveError;
      }
      if (url.Locations().size() == 0) {
        logger.msg(INFO, "Locations are missing in destination LFC URL");
        return DataStatus::WriteResolveError;
      }
    }
    int nbentries = 0;
    struct lfc_filereplica *entries = NULL;
    if (lfc_getreplica(url.Path().c_str(), NULL, NULL, &nbentries, &entries) != 0) {
      if (source || ((serrno != ENOENT) && (serrno != ENOTDIR))) {
        logger.msg(ERROR, "Error finding replicas: %s", sstrerror(serrno));
        lfc_endsess();
        return DataStatus::WriteResolveError;
      }
      nbentries = 0;
      entries = NULL;
    }
    else
      registered = true;

    if (url.Locations().size() == 0)
      for (int n = 0; n < nbentries; n++) {
        locations.push_back(URLLocation(entries[n].sfn, url.ConnectionURL()));
        logger.msg(DEBUG, "Adding location: %s - %s", url.ConnectionURL(), entries[n].sfn);
      }
    else
      for (std::list<URLLocation>::const_iterator loc = url.Locations().begin(); loc != url.Locations().end(); ++loc)
        for (int n = 0; n < nbentries; n++)
          if (strncmp(entries[n].sfn, loc->Name().c_str(), loc->Name().length()) == 0) {
            logger.msg(DEBUG, "Adding location: %s - %s", url.ConnectionURL(), entries[n].sfn);
            if (source)
              locations.push_back(URLLocation(entries[n].sfn, url.ConnectionURL()));
            else
              locations.push_back(URLLocation(*loc, url.ConnectionURL()));
            break;
          }
    if (entries)
      free(entries);

    struct lfc_filestatg st;
    if (lfc_statg(url.Path().c_str(), NULL, &st) == 0) {
      registered = true;
      SetSize(st.filesize);
      SetCreated(st.mtime);
      if (st.csumtype[0] && st.csumvalue[0]) {
        std::string csum = st.csumtype;
        if (csum == "MD")
          csum = "md5";
        csum += ":";
        csum += st.csumvalue;
        SetCheckSum(csum);
      }
      guid = st.guid;
    }
    lfc_endsess();
    if (!source) {
      if (locations.size() == 0) {
        logger.msg(INFO, "No locations found for destination");
        return DataStatus::WriteResolveError;
      }
      // Make pfns
      for (std::list<URLLocation>::iterator loc = locations.begin(); loc != locations.end(); loc++) {
        std::string u = loc->str();
        if (u.find_last_of("/") != u.length() - 1)
          u += "/"; // take off leading dirs of LFN
        std::string::size_type slash_index = url.Path().rfind("/", url.Path().length() + 1);
        if (slash_index != std::string::npos)
          u += url.Path().substr(slash_index + 1);
        else
          u += url.Path();
        *loc = URLLocation(u, loc->Name());
        logger.msg(DEBUG, "Using location: %s - %s", loc->Name(), loc->str());
      }
    }
    logger.msg(DEBUG, "meta_get_data: checksum: %s", GetCheckSum());
    logger.msg(DEBUG, "meta_get_data: size: %llu", GetSize());
    logger.msg(DEBUG, "meta_get_data: created: %s", GetCreated().str());

    // add any url options
    if (!url.CommonLocOptions().empty())
      for (std::list<URLLocation>::iterator loc = locations.begin(); loc != locations.end(); ++loc)
        for (std::map<std::string, std::string>::const_iterator i = url.CommonLocOptions().begin();
             i != url.CommonLocOptions().end(); i++)
          loc->AddOption(i->first, i->second, false);
    location = locations.begin();
    resolved = true;
    return DataStatus::Success;
  }

  DataStatus DataPointLFC::PreRegister(bool replication, bool force) {

#ifndef WITH_CTHREAD
    /* Initialize Cthread library - should be called before any LFC-API function */
    if (0 != Cthread_init()) {
      logger.msg(ERROR, "Cthread_init() error: %s", sstrerror(serrno));
      return DataStatus::PreRegisterError;
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
      return DataStatus::PreRegisterError;
    }
    if (guid.empty())
      GUID(guid);
    else if (!url.MetaDataOption("guid").empty()) {
      guid = url.MetaDataOption("guid");
      logger.msg(DEBUG, "Using supplied guid %s", guid);
    }
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

          logger.msg(DEBUG, "Creating LFC directory %s", dirname);
          if (lfc_mkdir(dirname.c_str(), 0775) != 0)
            if (serrno != EEXIST) {
              logger.msg(ERROR, "Error creating required LFC dirs: %s", sstrerror(serrno));
              lfc_endsess();
              return DataStatus::PreRegisterError;
            }
          slashpos = url.Path().find("/", slashpos + 1);
        }
        if (lfc_creatg(url.Path().c_str(), guid.c_str(),
                       S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP) != 0) {
          logger.msg(ERROR, "Error creating LFC entry: %s", sstrerror(serrno));
          lfc_endsess();
          return DataStatus::PreRegisterError;
        }
      }
      else {
        logger.msg(ERROR, "Error creating LFC entry: %s", sstrerror(serrno));
        lfc_endsess();
        return DataStatus::PreRegisterError;
      }
    }
    if (CheckCheckSum()) {
      std::string ckstype;
      std::string cksumvalue = GetCheckSum();
      std::string::size_type p = cksumvalue.find(':');
      if (p == std::string::npos)
        ckstype = "cksum";
      else {
        ckstype = cksumvalue.substr(0, p);
        cksumvalue = cksumvalue.substr(p + 1);
      }
      if (CheckSize())
        lfc_setfsizeg(guid.c_str(), GetSize(), ckstype.c_str(), const_cast<char*>(cksumvalue.c_str()));
      else
        lfc_setfsizeg(guid.c_str(), 0, ckstype.c_str(), const_cast<char*>(cksumvalue.c_str()));
    }
    else if (CheckSize())
      lfc_setfsizeg(guid.c_str(), GetSize(), NULL, NULL);

    lfc_endsess();
    return DataStatus::Success;
  }

  DataStatus DataPointLFC::PostRegister(bool replication) {

#ifndef WITH_CTHREAD
    /* Initialize Cthread library - should be called before any LFC-API function */
    if (0 != Cthread_init()) {
      logger.msg(ERROR, "Cthread_init() error: %s", sstrerror(serrno));
      return DataStatus::PostRegisterError;
    }
#endif

    if (guid.empty()) {
      logger.msg(ERROR, "No GUID defined for LFN - probably not preregistered");
      return DataStatus::PostRegisterError;
    }
    std::string pfn(location->str());
    if (lfc_startsess(const_cast<char*>(url.Host().c_str()), const_cast<char*>("ARC")) != 0) {
      logger.msg(ERROR, "Error starting session: %s", sstrerror(serrno));
      return DataStatus::PostRegisterError;
    }
    if (lfc_addreplica(guid.c_str(), NULL, location->Host().c_str(), pfn.c_str(), '-', 'P', NULL, NULL) != 0) {
      logger.msg(ERROR, "Error adding replica: %s", sstrerror(serrno));
      lfc_endsess();
      return DataStatus::PostRegisterError;
    }
    if (CheckCheckSum()) {
      std::string ckstype;
      std::string cksumvalue = GetCheckSum();
      std::string::size_type p = cksumvalue.find(':');
      if (p == std::string::npos)
        ckstype = "cksum";
      else {
        ckstype = cksumvalue.substr(0, p);
        if (ckstype == "md5")
          ckstype = "MD";
        if (ckstype == "ad")
          ckstype = "AD";
        cksumvalue = cksumvalue.substr(p + 1);
        logger.msg(DEBUG, "Entering checksum type %s, value %s, file size %llu", ckstype, cksumvalue, GetSize());
      }
      if (CheckSize())
        lfc_setfsizeg(guid.c_str(), GetSize(), ckstype.c_str(), const_cast<char*>(cksumvalue.c_str()));
      else
        lfc_setfsizeg(guid.c_str(), 0, ckstype.c_str(), const_cast<char*>(cksumvalue.c_str()));
    }
    else if (CheckSize())
      lfc_setfsizeg(guid.c_str(), GetSize(), NULL, NULL);

    lfc_endsess();
    return DataStatus::Success;
  }

  DataStatus DataPointLFC::PreUnregister(bool replication) {

#ifndef WITH_CTHREAD
    /* Initialize Cthread library - should be called before any LFC-API function */
    if (0 != Cthread_init()) {
      logger.msg(ERROR, "Cthread_init() error: %s", sstrerror(serrno));
      return DataStatus::UnregisterError;
    }
#endif

    if (replication)
      return DataStatus::Success;
    if (lfc_startsess(const_cast<char*>(url.Host().c_str()),
                      const_cast<char*>("ARC")) != 0) {
      logger.msg(ERROR, "Error starting session: %s", sstrerror(serrno));
      return DataStatus::UnregisterError;
    }
    if (!resolveGUIDToLFN()) {
      lfc_endsess();
      return DataStatus::UnregisterError;
    }
    if (lfc_unlink(url.Path().c_str()) != 0)
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
      return DataStatus::UnregisterError;
    }
#endif

    if (!all && (location == locations.end())) {
      logger.msg(ERROR, "Location is missing");
      return DataStatus::UnregisterError;
    }
    if (lfc_startsess(const_cast<char*>(url.Host().c_str()),
                      const_cast<char*>("ARC")) != 0) {
      logger.msg(ERROR, "Error starting session: %s", sstrerror(serrno));
      return DataStatus::UnregisterError;
    }
    if (!resolveGUIDToLFN()) {
      lfc_endsess();
      return DataStatus::UnregisterError;
    }
    if (all) {
      int nbentries = 0;
      struct lfc_filereplica *entries = NULL;
      if (lfc_getreplica(url.Path().c_str(), NULL, NULL, &nbentries, &entries) != 0) {
        lfc_endsess();
        if ((serrno == ENOENT) || (serrno == ENOTDIR)) {
          registered = false;
          locations.clear();
          location = locations.end();
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
      if (lfc_unlink(url.Path().c_str()) != 0) {
        if (serrno == EPERM) { // we have a directory
          if (lfc_rmdir(url.Path().c_str()) != 0) {
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
    }
    else if (lfc_delreplica(guid.c_str(), NULL, location->str().c_str()) != 0) {
      lfc_endsess();
      logger.msg(ERROR, "Failed to remove location from LFC: %s", sstrerror(serrno));
      return DataStatus::UnregisterError;
    }
    lfc_endsess();
    registered = false;
    locations.clear();
    location = locations.end();
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
      return DataStatus::ListError;
    }
#endif

    if (lfc_startsess(const_cast<char*>(url.Host().c_str()),
                      const_cast<char*>("ARC")) != 0) {
      logger.msg(ERROR, "Error starting session: %s", sstrerror(serrno));
      return DataStatus::ListError;
    }
    if (!resolveGUIDToLFN()) {
      lfc_endsess();
      return DataStatus::ListError;
    }
    // first stat the url and see if it is a file or directory
    struct lfc_filestatg st;
    if (lfc_statg(url.Path().c_str(), NULL, &st) != 0) {
      logger.msg(ERROR, "Error listing file or directory: %s", sstrerror(serrno));
      lfc_endsess();
      return DataStatus::ListError;
    }

    // if it's a directory, list entries
    if (st.filemode & S_IFDIR && !metadata) {
      lfc_DIR *dir = lfc_opendirxg(const_cast<char*>(url.Host().c_str()), url.Path().c_str(), NULL);
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
      std::list<FileInfo>::iterator f = files.insert(files.end(), FileInfo(url.Path().c_str()));
      f->SetMetaData("path", url.Path());
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

        if (lfc_getreplica(url.Path().c_str(), NULL, NULL, &nbentries, &entries) != 0) {
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

  bool DataPointLFC::resolveGUIDToLFN() {

    // check if guid is already defined
    if (!guid.empty())
      return true;

    // check for guid in the attributes
    if (url.MetaDataOption("guid").empty())
      return true;
    guid = url.MetaDataOption("guid");

    lfc_list listp;
    struct lfc_linkinfo *info = lfc_listlinks(NULL, (char*)guid.c_str(), CNS_LIST_BEGIN, &listp);
    if (!info) {
      logger.msg(ERROR, "Error finding LFN from guid %s: %s", guid, sstrerror(serrno));
      return false;
    }
    url = URL(url.Protocol() + "://" + url.Host() + "/" + std::string(info[0].path));
    logger.msg(DEBUG, "guid %s resolved to LFN %s", guid, url.Path());
    lfc_listlinks(NULL, (char*)guid.c_str(), CNS_LIST_END, &listp);
    return true;
  }

} // namespace Arc
