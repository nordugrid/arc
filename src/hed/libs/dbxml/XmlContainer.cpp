#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>

#include <sys/stat.h>
#include <sys/types.h>
#include <glibmm.h>
#ifdef WIN32
#include <arc/win32.h>
#endif

#include "XmlContainer.h"

namespace Arc
{

XmlContainer::XmlContainer(const std::string &db_path, const std::string &db_name):logger_(Arc::Logger::rootLogger, "DBXML")
{
    if (!Glib::file_test(db_path, Glib::FILE_TEST_IS_DIR)) {
        if (mkdir(db_path.c_str(), 0700) != 0) {
            logger_.msg(Arc::ERROR, "cannot create directory: %s", db_path);
            return;
        }
    }
    env_ = NULL;
    db_ = NULL;
    update_tid_ = NULL;
    env_ = new DbEnv(0);
    env_->open(db_path.c_str(), DB_CREATE | DB_INIT_MPOOL | DB_INIT_LOCK | DB_INIT_TXN | DB_RECOVER | DB_THREAD, 0644);
    db_ = new Db(env_, 0);
    db_->open(NULL, db_name.c_str(), NULL, DB_BTREE, DB_CREATE | DB_AUTO_COMMIT | DB_THREAD, 0644);
}

XmlContainer::~XmlContainer(void)
{
    if (db_ != NULL) {
        db_->close(0);
        delete db_;
        db_ = NULL;
    }
    if (env_ != NULL) {
        env_->close(0);
        delete env_;
        env_ = NULL;
    }
}

void
XmlContainer::put(const std::string &name, const std::string &content)
{
    void *k = (void *)name.c_str();
    void *v = (void *)content.c_str();
    Dbt key(k, name.size() + 1);
    Dbt value(v, content.size() + 1);
    DbTxn *tid = NULL;
    try {
        env_->txn_begin(NULL, &tid, 0);
    } catch (std::exception &e) {
        logger_.msg(Arc::ERROR, "Error init transaction: %s", e.what());
        return;
    }
    try {
        db_->put(tid, &key, &value, 0);
        tid->commit(0);
    } catch (DbException &e) {
        logger_.msg(Arc::ERROR, "Error to put data to xml database: %s", e.what());
        tid->abort();
    }
}

std::string
XmlContainer::get(const std::string &name)
{
    void *k = (void *)name.c_str();
    Dbt key(k, name.size() + 1);
    Dbt value;
    value.set_flags(DB_DBT_MALLOC);
    DbTxn *tid = NULL;
    try {
        env_->txn_begin(NULL, &tid, 0);
    } catch (std::exception &e) {
        logger_.msg(Arc::ERROR, "Error init transaction: %s", e.what());
        return "";
    }
    try {
        if (db_->get(tid, &key, &value, 0) != DB_NOTFOUND) {
            std::string ret((char *)value.get_data());
            free(value.get_data());
            tid->commit(0);
            return ret;
        }
        tid->commit(0);
        return "";
    } catch (DbException &e) {
        logger_.msg(Arc::ERROR, "Error to put data to xml database: %s", e.what());
        tid->abort();
    }
}

void
XmlContainer::del(const std::string &name)
{
    void *k = (void *)name.c_str();
    Dbt key(k, name.size() + 1);
    DbTxn *tid = NULL;
    try {
        env_->txn_begin(NULL, &tid, 0);
    } catch (std::exception &e) {
        logger_.msg(Arc::ERROR, "Error init transaction: %s", e.what());
        return;
    }
    try {
        db_->del(tid, &key, 0);
        tid->commit(0);
    } catch (DbException &e) {
        logger_.msg(Arc::ERROR, "Error to put data to xml database: %s", e.what());
        tid->abort();
    }

}

void
XmlContainer::start_update(void)
{
}

void 
XmlContainer::end_update(void)
{
}

std::vector<std::string> 
XmlContainer::get_doc_names(void)
{
    std::vector<std::string> result;
    Dbc *cursor = NULL;
    DbTxn *tid;
    try {
        env_->txn_begin(NULL, &tid, 0);
    } catch (std::exception &e) {
        logger_.msg(Arc::ERROR, "Error init transaction: %s", e.what());
        return result;
    }
    try {
        db_->cursor(tid, &cursor, 0);
        Dbt key, value;
        key.set_flags(0);
        value.set_flags(0);
        int ret;
        for (;;) {
            ret = cursor->get(&key, &value, DB_NEXT);
            if (ret == DB_NOTFOUND) {
                break;
            }
            char *k = (char *)key.get_data();
            result.push_back(std::string(k));
        }
        cursor->close();
        tid->commit(0);
    } catch (DbException &e) {
        logger_.msg(Arc::ERROR, "Error during the transaction: %s", e.what());
        if (cursor != NULL) {
            cursor->close();
        }
        tid->abort();
    }
    return result;
}

} // namespace

