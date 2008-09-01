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
    // setup internal deadlock detection mechanizm
    env_->set_lk_detect(DB_LOCK_DEFAULT);
    db_ = new Db(env_, 0);
    DbTxn *tid = NULL;
    try {
#if (DB_VERSION_MAJOR < 4) || (DB_VERSION_MAJOR == 4 && DB_VERSION_MINOR == 0)
        db_->open(db_name.c_str(), NULL, DB_BTREE, DB_CREATE | DB_THREAD, 0644);
#else
        env_->txn_begin(NULL, &tid, 0);
        db_->open(tid, db_name.c_str(), NULL, DB_BTREE, DB_CREATE | DB_THREAD, 0644);
        tid->commit(0);
#endif
    } catch (DbException &e) {
        try {
            tid->abort();
            logger_.msg(Arc::ERROR, "Cannot open database");
        } catch (DbException &e) {
            logger_.msg(Arc::ERROR, "Cannot abort transaction %s", e.what()); 
        }
        db_ = NULL;
        return;
    }
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

bool
XmlContainer::put(const std::string &name, const std::string &content)
{
    void *k = (void *)name.c_str();
    void *v = (void *)content.c_str();
    Dbt key(k, name.size() + 1);
    Dbt value(v, content.size() + 1);
    DbTxn *tid = NULL;
    while (true) {
        try {
            env_->txn_begin(NULL, &tid, 0);
            db_->put(tid, &key, &value, 0);
            tid->commit(0);
            return true;
        } catch (DbDeadlockException &e) {
            try { 
                tid->abort();
                logger_.msg(Arc::INFO, "put: deadlock handling: try again");
            } catch (DbException &e) {
                logger_.msg(Arc::ERROR, "put: cannot abort transaction: %s", e.what());
                return false;
            }
        } catch (DbException &e) {
            logger_.msg(Arc::ERROR, "put: %s", e.what());
            try {
                tid->abort();
            } catch (DbException &e) {
                logger_.msg(Arc::ERROR, "put: cannot abort transaction: %s", e.what());
            }
            return false;
        }
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
    while (true) {
        try {
            env_->txn_begin(NULL, &tid, 0);
            if (db_->get(tid, &key, &value, 0) != DB_NOTFOUND) {
                std::string ret((char *)value.get_data());
                free(value.get_data());
                tid->commit(0);
                return ret;
            }
            tid->commit(0);
            return "";
        } catch (DbDeadlockException &e) {
            try {
                tid->abort();
                logger_.msg(Arc::INFO, "get: deadlock handling, try again");
            } catch (DbException &e) {
                logger_.msg(Arc::ERROR, "get: cannot abort transaction: %s", e.what());
                return "";
            }
        } catch (DbException &e) {
            logger_.msg(Arc::ERROR, "get: %s", e.what());
            try {
                tid->abort();
            } catch (DbException &e) {
                logger_.msg(Arc::ERROR, "get: cannot abort transaction: %s", e.what());
            }
            return "";
        }
    }
}

void
XmlContainer::del(const std::string &name)
{
    void *k = (void *)name.c_str();
    Dbt key(k, name.size() + 1);
    DbTxn *tid = NULL;
    while (true) {
        try {
            env_->txn_begin(NULL, &tid, 0);
            db_->del(tid, &key, 0);
            tid->commit(0);
        } catch (DbDeadlockException &e) {
            try {
                tid->abort();
                logger_.msg(Arc::INFO, "del: deadlock handling, try again");
            } catch (DbException &e) {
                logger_.msg(Arc::ERROR, "del: cannot abort transaction: %s", e.what());
                return;
            }
        } catch (DbException &e) {
            logger_.msg(Arc::ERROR, "del: %s", e.what());
            try {
                tid->abort();
            } catch (DbException &e) {
                logger_.msg(Arc::ERROR, "del: cannot abort transaction: %s", e.what());
            }
            return;
        }
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
    Dbc *cursor = NULL;
    Dbt key, value;
    key.set_flags(0);
    value.set_flags(0);
    DbTxn *tid;
    std::vector<std::string> empty_result;
    while (true) {
        std::vector<std::string> result;
        env_->txn_begin(NULL, &tid, 0);
        try {
            db_->cursor(tid, &cursor, 0);
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
            return result;
        } catch (DbDeadlockException &e) {
            try {
                if (cursor != NULL) {
                    cursor->close();
                }
                tid->abort();
                logger_.msg(Arc::INFO, "get_doc_name: deadlock handling, try again");
            } catch (DbException &e) {
                logger_.msg(Arc::ERROR, "get_doc_names: cannot abort transaction: %s", e.what());
                return empty_result;
            }
        } catch (DbException &e) {
            logger_.msg(Arc::ERROR, "Error during the transaction: %s", e.what());
            try {
                if (cursor != NULL) {
                    cursor->close();
                }
                tid->abort();
            } catch (DbException &e) {
                logger_.msg(Arc::ERROR, "get_doc_names: cannot abort transaction: %s", e.what());
            }
            return empty_result;
        }
    }
}

} // namespace

