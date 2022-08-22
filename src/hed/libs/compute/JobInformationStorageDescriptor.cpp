// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "JobInformationStorageXML.h"
#ifdef SQLITEJSTORE_ENABLED
#include "JobInformationStorageSQLite.h"
#endif

namespace Arc {
  
  JobInformationStorageDescriptor JobInformationStorage::AVAILABLE_TYPES[] = {
#ifdef SQLITEJSTORE_ENABLED
    { "SQLITE", &JobInformationStorageSQLite::Instance },
#endif
    { "XML", &JobInformationStorageXML::Instance },
    { NULL, NULL }
  };
} // namespace Arc
