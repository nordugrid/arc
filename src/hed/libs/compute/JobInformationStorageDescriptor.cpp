// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "JobInformationStorageXML.h"
#ifdef DBJSTORE_ENABLED
#include "JobInformationStorageBDB.h"
#endif
#ifdef HAVE_SQLITE
#include "JobInformationStorageSQLite.h"
#endif

namespace Arc {
  
  JobInformationStorageDescriptor JobInformationStorage::AVAILABLE_TYPES[] = {
#ifdef DBJSTORE_ENABLED
    { "BDB", &JobInformationStorageBDB::Instance },
#endif
#ifdef HAVE_SQLITE
    { "SQLITE", &JobInformationStorageSQLite::Instance },
#endif
    { "XML", &JobInformationStorageXML::Instance },
    { NULL, NULL }
  };
} // namespace Arc
