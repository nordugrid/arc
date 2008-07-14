#ifndef __ARC_XML_DATABASE_H__
#define __ARC_XML_DATABASE_H__

#include <string>
#include <map>
#include <arc/XMLNode.h>
#include "XmlContainer.h"

namespace Arc
{

class XmlDatabase
{
    private:
        XmlContainer *container_;

    public:
        XmlDatabase(const std::string &db_path, const std::string &db_name);
        ~XmlDatabase();
        void put(const std::string &name, const std::string &content);
        void put(const std::string &name, Arc::XMLNode &doc);
        void get(const std::string &name, Arc::XMLNode &doc);
        void del(const std::string &name);
        Arc::XMLNodeContainer query(const std::string &name, 
                                     const std::string &query);
        std::map<std::string, Arc::XMLNodeContainer> queryAll(const std::string &query);
        void update(const std::string &name, const std::string &query, Arc::XMLNode &new_value);
};

}

#endif
