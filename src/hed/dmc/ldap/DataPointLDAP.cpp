// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/StringConv.h>
#include <arc/UserConfig.h>
#include <arc/data/DataBuffer.h>

#include "DataPointLDAP.h"
#include "LDAPQuery.h"

namespace ArcDMCLDAP {

  using namespace Arc;

  Logger DataPointLDAP::logger(Logger::getRootLogger(), "DataPoint.LDAP");

  DataPointLDAP::DataPointLDAP(const URL& url, const UserConfig& usercfg, PluginArgument* parg)
    : DataPointDirect(url, usercfg, parg) {}

  DataPointLDAP::~DataPointLDAP() {
    StopReading();
    StopWriting();
  }

  Plugin* DataPointLDAP::Instance(PluginArgument *arg) {
    DataPointPluginArgument *dmcarg =
      dynamic_cast<DataPointPluginArgument*>(arg);
    if (!dmcarg) return NULL;
    if (((const URL&)(*dmcarg)).Protocol() != "ldap") return NULL;
    Glib::Module* module = dmcarg->get_module();
    PluginsFactory* factory = dmcarg->get_factory();
    if(!(factory && module)) {
      logger.msg(ERROR, "Missing reference to factory and/or module. Currently safe unloading of LDAP DMC is not supported. Report to developers.");
      return NULL;
    }
    // It looks like this DMC can't ensure all started threads are stopped
    // by its destructor. So current hackish solution is to keep code in
    // memory.
    // TODO: handle threads properly
    // TODO: provide generic solution for holding plugin in memory as long
    //   as plugins/related obbjects are still active.
    factory->makePersistent(module);
    return new DataPointLDAP(*dmcarg, *dmcarg, dmcarg);
  }

  DataStatus DataPointLDAP::Check(bool check_meta) {
    return DataStatus::Success;
  }

  DataStatus DataPointLDAP::Remove() {
    return DataStatus(DataStatus::UnimplementedError, EOPNOTSUPP);
  }

  void DataPointLDAP::CallBack(const std::string& attr,
                               const std::string& value,
                               void *ref) {
    DataPointLDAP& point = *(DataPointLDAP*)ref;

    if (attr == "dn") {
      point.entry = point.node;

      std::string path = "";
      std::string attr_val = "";
      std::string::size_type pos_o = value.size();
      while (pos_o != std::string::npos) {
        std::string::size_type pos_n = (pos_o>0)?value.rfind(',', pos_o-1):std::string::npos;
        if(pos_n == std::string::npos) {
          attr_val = value.substr(0,pos_o);
        } else {
          attr_val = value.substr(pos_n+1,pos_o-pos_n-1);
        }
        pos_o = pos_n;
        attr_val = trim(attr_val, " ");
        path+=attr_val+",";

        std::map<std::string, XMLNode>::iterator c_path = point.dn_cache.find(path);
        if (c_path != point.dn_cache.end()) {
          point.entry = c_path->second;
        }
        else {
          std::string::size_type pos_eq = attr_val.find('=');
          if(pos_eq != std::string::npos) {
            point.entry = point.entry.NewChild(trim(attr_val.substr(0, pos_eq), " ")) = trim(attr_val.substr(pos_eq + 1), " ");
          } else {
            point.entry = point.entry.NewChild(trim(attr_val, " "));
          };
          point.dn_cache.insert(std::make_pair(path, point.entry));
        }
      }
    }
    else {
      point.entry.NewChild(attr) = value;
    }
  }

  DataStatus DataPointLDAP::StartReading(DataBuffer& buf) {
    if (buffer) return DataStatus::IsReadingError;
    buffer = &buf;
    LDAPQuery q(url.Host(), url.Port(), usercfg.Timeout());
    int res = q.Query(url.Path(), url.LDAPFilter(), url.LDAPAttributes(),
                      url.LDAPScope());
    if (res != 0) {
      buffer = NULL;
      return DataStatus(DataStatus::ReadStartError, (res == 1) ? ETIMEDOUT : ECONNREFUSED);
    }
    NS ns;
    XMLNode(ns, "LDAPQueryResult").New(node);
    res = q.Result(CallBack, this);
    if (res != 0) {
      buffer = NULL;
      return DataStatus(DataStatus::ReadStartError, (res == 1) ? ETIMEDOUT : ECONNREFUSED);
    }
    if(!CreateThreadFunction(&ReadThread, this, &thread_cnt)) {
      buffer = NULL;
      return DataStatus(DataStatus::ReadStartError);
    }
    return DataStatus::Success;
  }

  DataStatus DataPointLDAP::StopReading() {
    if (!buffer) return DataStatus::ReadStopError;
    if(!buffer->eof_read()) buffer->error_read(true);
    buffer = NULL;
    thread_cnt.wait();
    return DataStatus::Success;
  }

  DataStatus DataPointLDAP::StartWriting(DataBuffer&, DataCallback*) {
    return DataStatus(DataStatus::UnimplementedError, EOPNOTSUPP);
  }

  DataStatus DataPointLDAP::StopWriting() {
    return DataStatus(DataStatus::UnimplementedError, EOPNOTSUPP);
  }

  DataStatus DataPointLDAP::Stat(FileInfo& file, DataPoint::DataPointInfoType verb) {
    return DataStatus(DataStatus::UnimplementedError, EOPNOTSUPP);
  }

  DataStatus DataPointLDAP::List(std::list<FileInfo>& file, DataPoint::DataPointInfoType verb) {
    // TODO: Implement through Read
    return DataStatus(DataStatus::UnimplementedError, EOPNOTSUPP);
  }

  void DataPointLDAP::ReadThread(void *arg) {
    DataPointLDAP& point = *(DataPointLDAP*)arg;
    std::string text;
    point.node.GetDoc(text);
    std::string::size_type length = text.size();
    unsigned long long int pos = 0;
    int transfer_handle = -1;
    do {
      unsigned int transfer_size = 0;
      if(!point.buffer->for_read(transfer_handle, transfer_size, true)) break;
      if (length < transfer_size) transfer_size = length;
      memcpy((*point.buffer)[transfer_handle], &text[pos], transfer_size);
      point.buffer->is_read(transfer_handle, transfer_size, pos);
      length -= transfer_size;
      pos += transfer_size;
    } while (length > 0);
    point.buffer->eof_read(true);
  }

} // namespace ArcDMCLDAP

extern Arc::PluginDescriptor const ARC_PLUGINS_TABLE_NAME[] = {
  { "ldap", "HED:DMC", "Lightweight Directory Access Protocol", 0, &ArcDMCLDAP::DataPointLDAP::Instance },
  { NULL, NULL, NULL, 0, NULL }
};

extern "C" {
  void ARC_MODULE_CONSTRUCTOR_NAME(Glib::Module* module, Arc::ModuleManager* manager) {
    if(manager && module) {
      manager->makePersistent(module);
    };
  }
}

