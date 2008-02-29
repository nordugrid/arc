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
    stop_reading();
    stop_writing();
    deinit_handle();
  }

  bool DataPointLDAP::check() {
    if(!DataPointDirect::check())
      return false;
    return true;
  }

  bool DataPointLDAP::remove() {
    if(!DataPointDirect::remove())
      return false;
    return false;
  }

  void DataPointLDAP::CallBack(const std::string& attr,
			       const std::string& value,
			       void *ref) {
    DataPointLDAP& point = *(DataPointLDAP*) ref;
    if(attr == "dn") {
      point.entry = point.node;

      std::list<std::pair<std::string, std::string> > pairs;
      std::string::size_type pos = 0;

      while(pos != std::string::npos) {
	std::string::size_type pos2 = value.find(',', pos);
	std::string attr = (pos2 == std::string::npos ?
                            value.substr(pos) :
			    value.substr(pos, pos2 - pos));
	pos = pos2;
	if(pos != std::string::npos)
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
	if(!found)
	  point.entry = point.entry.NewChild(it->first) = it->second;
      }
    }
    else
      point.entry.NewChild(attr) = value;
  }

  bool DataPointLDAP::start_reading(DataBufferPar& buf) {
    buffer = &buf;
    LDAPQuery q(url.Host(), url.Port());
    q.Query(url.Path(), url.LDAPFilter(), url.LDAPAttributes(),
	    url.LDAPScope() == "base" ? LDAPQuery::base :
	    url.LDAPScope() == "one" ? LDAPQuery::onelevel : LDAPQuery::subtree);
    NS ns;
    XMLNode(ns, "LDAPQueryResult").New(node);
    q.Result(CallBack, this);
    CreateThreadFunction(&ReadThread, this);
    return true;
  }

  bool DataPointLDAP::stop_reading() {
    if(!buffer) return false;
    buffer = NULL;
    return true;
  }

  bool DataPointLDAP::start_writing(DataBufferPar& buf,
                                    DataCallback *space_cb) {
    return false;
  }

  bool DataPointLDAP::stop_writing() {
    return false;
  }

  bool DataPointLDAP::list_files(std::list<FileInfo>& files, bool resolve) {
    return false;
  }

  bool DataPointLDAP::analyze(analyze_t& arg) {
    return false;
  }

  void DataPointLDAP::ReadThread(void *arg) {
    DataPointLDAP& point = *(DataPointLDAP*)arg;
    std::string text;
    point.node.GetDoc(text);
    const char *buf = text.c_str();
    std::string::size_type length = text.size();
    unsigned long long int pos = 0;
    int transfer_handle = -1;
    do {
      unsigned int transfer_size = 0;
      point.buffer->for_read(transfer_handle, transfer_size, true);
      if(length < transfer_size) transfer_size = length;
      memcpy((*point.buffer)[transfer_handle], &text[pos], transfer_size);
      point.buffer->is_read(transfer_handle, transfer_size, pos);
      length -= transfer_size;
      pos += transfer_size;
    } while (length > 0);
    point.buffer->eof_read(true);
  }

} // namespace Arc
