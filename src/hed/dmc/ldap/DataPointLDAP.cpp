#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "DataPointLDAP.h"
#include "LDAPQuery.h"

#include <arc/data/DataBufferPar.h>

#include <arc/URL.h>

namespace Arc {

  Logger DataPointLDAP::logger(DataPoint::logger, "LDAP");

  DataPointLDAP::DataPointLDAP(const URL& url) : DataPointDirect(url) {}

  DataPointLDAP::~DataPointLDAP() {
    StopReading();
    StopWriting();
  }

  DataStatus DataPointLDAP::Check() {
    return DataStatus::Success;
  }

  DataStatus DataPointLDAP::Remove() {
    return DataStatus::UnimplementedError;
  }

  void DataPointLDAP::CallBack(const std::string& attr,
                               const std::string& value,
                               void *ref) {
    DataPointLDAP& point = *(DataPointLDAP*) ref;
    if (attr == "dn") {
      point.entry = point.node;

      std::list<std::pair<std::string, std::string> > pairs;
      std::string::size_type pos = 0;

      while(pos != std::string::npos) {
        std::string::size_type pos2 = value.find(',', pos);
        std::string attr = (pos2 == std::string::npos ?
                            value.substr(pos) :
                            value.substr(pos, pos2 - pos));
        pos = pos2;
        if (pos != std::string::npos)
          pos++;
        pos2 = attr.find('=');
        std::string s1 = attr.substr(0, pos2);
        std::string s2 = attr.substr(pos2 + 1);

        while (s1[0] == ' ') s1 = s1.erase(0, 1);
        while (s1[s1.size() - 1] == ' ') s1 = s1.erase(s1.size() - 1);
        while (s2[0] == ' ') s2 = s2.erase(0, 1);
        while (s2[s2.size() - 1] == ' ') s2 = s2.erase(s2.size() - 1);

        pairs.push_back(std::make_pair(s1, s2));
      }

      for (std::list<std::pair<std::string, std::string> >::reverse_iterator
             it = pairs.rbegin(); it != pairs.rend(); it++) {
        bool found = false;
        for (int i = 0; point.entry[it->first][i]; i++) {
          if ((std::string) point.entry[it->first][i] == it->second) {
            point.entry = point.entry[it->first][i];
            found = true;
            break;
          }
        }
        if (!found)
          point.entry = point.entry.NewChild(it->first) = it->second;
      }
    }
    else
      point.entry.NewChild(attr) = value;
  }

  DataStatus DataPointLDAP::StartReading(DataBufferPar& buf) {
    buffer = &buf;
    LDAPQuery q(url.Host(), url.Port());
    q.Query(url.Path(), url.LDAPFilter(), url.LDAPAttributes(),
            url.LDAPScope());
    NS ns;
    XMLNode(ns, "LDAPQueryResult").New(node);
    q.Result(CallBack, this);
    CreateThreadFunction(&ReadThread, this);
    return DataStatus::Success;
  }

  DataStatus DataPointLDAP::StopReading() {
    if (!buffer)
      return DataStatus::ReadStopError;
    buffer = NULL;
    return DataStatus::Success;
  }

  DataStatus DataPointLDAP::StartWriting(DataBufferPar&, DataCallback*) {
    return DataStatus::UnimplementedError;
  }

  DataStatus DataPointLDAP::StopWriting() {
    return DataStatus::UnimplementedError;
  }

  DataStatus DataPointLDAP::ListFiles(std::list<FileInfo>&, bool) {
    return DataStatus::UnimplementedError;
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
      point.buffer->for_read(transfer_handle, transfer_size, true);
      if (length < transfer_size)
        transfer_size = length;
      memcpy((*point.buffer)[transfer_handle], &text[pos], transfer_size);
      point.buffer->is_read(transfer_handle, transfer_size, pos);
      length -= transfer_size;
      pos += transfer_size;
    } while (length > 0);
    point.buffer->eof_read(true);
  }

} // namespace Arc
