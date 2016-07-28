#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <cstdio>
#include <fstream>
#include <pwd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <arc/DateTime.h>
#include <arc/FileUtils.h>
#include <arc/Logger.h>
#include <arc/OptionParser.h>
#include <arc/StringConv.h>

#include "conf/GMConfig.h"
#include "../delegation/DelegationStore.h"
#include "../delegation/FileRecordBDB.h"
#include "../delegation/FileRecordSQLite.h"

using namespace ARex;

int main(int argc, char* argv[]) {

  // stderr destination for error messages
  Arc::LogStream logcerr(std::cerr);
  Arc::Logger::getRootLogger().addDestination(logcerr);
  Arc::Logger::getRootLogger().setThreshold(Arc::DEBUG);
  
  Arc::OptionParser options(" ",
                            istring("gm-delegations-converter changes "
                                    "format of delegation database."));
  
  std::string conf_file;
  options.AddOption('c', "conffile",
                    istring("use specified configuration file"),
                    istring("file"), conf_file);
  
  std::string control_dir;
  options.AddOption('d', "controldir",
                    istring("read information from specified control directory"),
                    istring("dir"), control_dir);
                    
  bool show_delegs = false;
  options.AddOption('e', "listdelegs",
		    istring("print list of available delegation IDs"),
		    show_delegs);

  std::list<std::string> show_deleg_jobs;
  options.AddOption('D', "showdelegjob",
                    istring("print main delegation token of specified Job ID(s)"),
                    istring("job id"), show_deleg_jobs);

  std::string output_format;
  options.AddOption('o', "output",
                    istring("convert current databse into specified output format [bdb|sqlite]"),
                    istring("database format"), output_format);


  std::list<std::string> params = options.Parse(argc, argv);

  GMConfig config;
  if (!conf_file.empty()) config.SetConfigFile(conf_file);
  
  std::cout << "Using configuration at " << config.ConfigFile() << std::endl;
  if(!config.Load()) exit(1);
  
  if (!control_dir.empty()) config.SetControlDir(control_dir);

  config.Print();

  DelegationStore::DbType deleg_db_type = DelegationStore::DbBerkeley;
  DelegationStore::DbType deleg_db_type_out = DelegationStore::DbSQLite;
  switch(config.DelegationDBType()) {
   case GMConfig::deleg_db_bdb:
    deleg_db_type = DelegationStore::DbBerkeley;
    deleg_db_type_out = DelegationStore::DbSQLite;
    break;
   case GMConfig::deleg_db_sqlite:
    deleg_db_type = DelegationStore::DbSQLite;
    deleg_db_type_out = DelegationStore::DbBerkeley;
    break;
  };

  if(!output_format.empty()) {
    if(output_format == "bdb") {
      deleg_db_type_out = DelegationStore::DbBerkeley;
    } else if(output_format == "sqlite") {
      deleg_db_type_out = DelegationStore::DbSQLite;
    } else {
      std::cerr << "Unknown output database type requested - " << output_format << std::endl;
      exit(-1);
    };
  };

  switch(deleg_db_type) {
   case DelegationStore::DbBerkeley:
    std::cout << "Using original database type - Berkeley DB" << std::endl;
    break;
   case DelegationStore::DbSQLite:
    std::cout << "Using original database type - SQLite" << std::endl;
    break;
   default:
    std::cerr << "Failed identifying source database type" << std::endl;
    exit(-1);
  };
  switch(deleg_db_type_out) {
   case DelegationStore::DbBerkeley:
    std::cout << "Using output database type - Berkeley DB" << std::endl;
    break;
   case DelegationStore::DbSQLite:
    std::cout << "Using output database type - SQLite" << std::endl;
    break;
   default:
    std::cerr << "Failed identifying output database type" << std::endl;
    exit(-1);
  };

  FileRecord* source_db = NULL;
  std::string delegation_dir = config.DelegationDir();
  switch(deleg_db_type) {
   case DelegationStore::DbBerkeley:
    source_db = new FileRecordBDB(delegation_dir, false);
    break;
   case DelegationStore::DbSQLite:
    source_db = new FileRecordSQLite(delegation_dir, false);
    break;
  };
  if((!source_db) || (!*source_db)) {
    std::cerr << "Failed opening source database" << std::endl;
    exit(-1);
  };
  

  FileRecord* output_db = NULL;
  std::string delegation_dir_output;
  if(!Arc::TmpDirCreate(delegation_dir_output)) {
    std::cerr << "Failed to create temporary directory for new database" << std::endl;
    exit(-1);
  };
  switch(deleg_db_type_out) {
   case DelegationStore::DbBerkeley:
    output_db = new FileRecordBDB(delegation_dir_output, true);
    break;
   case DelegationStore::DbSQLite:
    output_db = new FileRecordSQLite(delegation_dir_output, true);
    break;
  };
  if((!output_db) || (!*output_db)) {
    std::cerr << "Failed creating output database" << std::endl;
    exit(-1);
  };

  // Copy database content record by record
  FileRecord::Iterator* prec = source_db->NewIterator();
  if(!prec) {
    std::cerr << "Failed opening iterator for source database" << std::endl;
    exit(-1);
  };
  bool copy_success = true;
  for(;*prec;++(*prec)) {
    std::string uid = prec->uid();
    std::string id = prec->id();
    std::string owner = prec->owner();
    std::list<std::string> meta = prec->meta();
    if(!output_db->Add(uid, id, owner, meta)) {
      copy_success = false;
      std::cerr << "Failed copying record " << id << ", " << owner << " - "
                << output_db->Error() << std::endl;
    };
    // todo: copy locks
  };
  delete prec;
  delete source_db;
  delete output_db;
  if(!copy_success) {
    Arc::DirDelete(delegation_dir_output, true);
    exit(-1);
  };

  // Move generated database (first delete old one)
  try {
    Glib::Dir dir(delegation_dir);
    std::string name;
    while ((name = dir.read_name()) != "") {
      std::string fullpath(delegation_dir);
      fullpath += G_DIR_SEPARATOR_S + name;
      struct stat st;
      if (::lstat(fullpath.c_str(), &st) == 0) {
        if(!S_ISDIR(st.st_mode)) {
          if(!Arc::FileDelete(fullpath.c_str())) {
            std::cerr << "Failed deleting source database - file " << fullpath << std::endl;
            std::cerr << "You MUST manually copy content of " << delegation_dir_output << 
                         " into " << delegation_dir << std::endl;
            exit(-1);
          };
        };
      };
    };
  } catch(Glib::FileError& e) {
    std::cerr << "Failed deleting source database" << std::endl;
    std::cerr << "You MUST manually copy content of " << delegation_dir_output << 
                 " into " << delegation_dir << std::endl;
    exit(-1);
  };

  try {
    Glib::Dir dir(delegation_dir_output);
    std::string name;
    while ((name = dir.read_name()) != "") {
      std::string fullpath_source(delegation_dir_output);
      fullpath_source += G_DIR_SEPARATOR_S + name;
      std::string fullpath_dest(delegation_dir);
      fullpath_dest += G_DIR_SEPARATOR_S + name;
      if(!Arc::FileCopy(fullpath_source, fullpath_dest)) {
        std::cerr << "Failed copying new database - file " << fullpath_source << std::endl;
        std::cerr << "You MUST manually copy content of " << delegation_dir_output << 
                     " into " << delegation_dir << std::endl;
        exit(-1);
      };
    };
  } catch(Glib::FileError& e) {
    std::cerr << "Failed copying new database" << std::endl;
    std::cerr << "You MUST manually copy content of " << delegation_dir_output << 
                 " into " << delegation_dir << std::endl;
    exit(-1);
  };

  Arc::DirDelete(delegation_dir_output, false);
  return 0;
}
