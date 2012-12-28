// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_EXTRACTOR_H__
#define __ARC_EXTRACTOR_H__

#include <string>

#include <arc/Logger.h>
#include <arc/XMLNode.h>

namespace Arc {
  class Extractor {
  public:
    Extractor() : logger(NULL) {}
    Extractor(XMLNode node, const std::string& prefix = "", const std::string& type = "", Logger* logger = NULL) : node(node), prefix(prefix), type(type), logger(logger) {}

    std::string get(const std::string& name) const {
      std::string value = node[type + prefix + name];
      if (value.empty()) {
        value = (std::string)node[type + name];
      }
      if (logger) logger->msg(DEBUG, "Extractor[%s] (%s): %s = %s", type, prefix, name, value);
      return value;
    }

    std::string operator[](const std::string& name) const {
      return get(name);
    }

    std::string operator[](const char* name) const {
      return get(name);
    }

    operator bool() const { return (bool)node; }

    bool set(const std::string& name, std::string& string, const std::string& undefined = "") const {
      std::string value = get(name);
      if (!value.empty() && value != undefined) {
        string = value;
        return true;
      } 
      return false;
    }

    bool set(const std::string& name, Period& period, const std::string& undefined = "") const {
      std::string value = get(name);
      if (!value.empty() && value != undefined) {
        period = Period(value);
        return true;
      } 
      return false;
    }

    bool set(const std::string& name, Time& time, const std::string& undefined = "") const {
      std::string value = get(name);
      if (!value.empty() && value != undefined) {
        time = Time(value);
        return true;
      }
      return false;
    }

    bool set(const std::string& name, int& integer, int undefined = -1) const {
      const std::string value = get(name);
      int tempInteger;
      if (value.empty() || !stringto(value, tempInteger) || tempInteger == undefined) return false;
      integer = tempInteger;
      return true;
    }

    bool set(const std::string& name, float& number) {
      std::string value = get(name);
      return !value.empty() && stringto(value, number);
    }

    bool set(const std::string& name, double& number) {
      std::string value = get(name);
      return !value.empty() && stringto(value, number);
    }

    bool set(const std::string& name, URL& url) {
      std::string value = get(name);
      if (!value.empty()) {
        url = URL(value);
        return true;
      } else {
        return false;
      }
    }

    bool set(const std::string& name, bool& boolean) {
      std::string value = get(name);
      if (!value.empty()) {
        boolean = (value == "TRUE");
        return true;
      } else {
        return false;
      }
    }

    bool set(const std::string& name, std::list<std::string>& list) {
      XMLNodeList nodelist = node.Path(type + prefix + name);
      if (nodelist.empty()) {
        nodelist = node.Path(type + name);
      }
      if (nodelist.empty()) {
        return false;
      }
      list.clear();
      for(XMLNodeList::iterator it = nodelist.begin(); it != nodelist.end(); it++) {
        std::string value = *it;
        list.push_back(value);
        if (logger) logger->msg(DEBUG, "Extractor[%s] (%s): %s contains %s", type, prefix, name, value);
      }
      return true;
    }

    bool set(const std::string& name, std::set<std::string>& list) {
      XMLNodeList nodelist = node.Path(type + prefix + name);
      if (nodelist.empty()) {
        nodelist = node.Path(type + name);
      }
      if (nodelist.empty()) {
        return false;
      }
      list.clear();
      for(XMLNodeList::iterator it = nodelist.begin(); it != nodelist.end(); it++) {
        std::string value = *it;
        list.insert(value);
        if (logger) logger->msg(DEBUG, "Extractor[%s] (%s): %s contains %s", type, prefix, name, value);
      }
      return true;
    }

    static Extractor First(XMLNode& node, const std::string& objectClass, const std::string& type = "", Logger* logger = NULL) {
      XMLNodeList objects = node.XPathLookup("//*[objectClass='" + type + objectClass + "']", NS());
      if(objects.empty()) return Extractor();
      return Extractor(objects.front(), objectClass, type, logger);
    }

    static Extractor First(Extractor& e, const std::string& objectClass) {
      return First(e.node, objectClass, e.type, e.logger);
    }

    static std::list<Extractor> All(XMLNode& node, const std::string& objectClass, const std::string& type = "", Logger* logger = NULL) {
      std::list<XMLNode> objects = node.XPathLookup("//*[objectClass='" + type + objectClass + "']", NS());
      std::list<Extractor> extractors;
      for (std::list<XMLNode>::iterator it = objects.begin(); it != objects.end(); ++it) {
        extractors.push_back(Extractor(*it, objectClass, type, logger));
      }
      return extractors;
    }

    static std::list<Extractor> All(Extractor& e, const std::string& objectClass) {
      return All(e.node, objectClass, e.type, e.logger);
    }


    XMLNode node;
    std::string prefix;
    std::string type;
    Logger *logger;
  };

} // namespace Arc

#endif // __ARC_EXTRACTOR_H__
