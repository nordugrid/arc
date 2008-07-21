#ifndef __ARC_XML_CONTAINER_H__
#define __ARC_XML_CONTAINER_H__

#include <string>
#include <vector>

#ifdef HAVE_DB_CXX_H
#include <db_cxx.h>
#endif

#ifdef HAVE_DB4_DB_CXX_H
#include <db4/db_cxx.h>
#endif

#ifdef HAVE_DB44_DB_CXX_H
#include <db44/db_cxx.h>
#endif

#include <arc/XMLNode.h>
#include <arc/Logger.h>

namespace Arc
{

class XmlContainer
{
    private:
        DbEnv *env_;
        Db *db_;
        DbTxn *update_tid_;
        Arc::Logger logger_;

    public:
        XmlContainer(const std::string &db_path, const std::string &db_name);
        ~XmlContainer();
        void put(const std::string &name, const std::string &content);
        std::string get(const std::string &name);
        void del(const std::string &name);
        std::vector<std::string> get_doc_names(void);
        void start_update(void);
        void end_update(void);
};

}
#endif
