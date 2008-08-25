#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/ByteArray.h>
#include "job_queue.h"

namespace Arc
{

static JobSelector *default_selector = new JobSelector();

JobQueueIterator::JobQueueIterator()
{
    tid_ = NULL;
    cursor_ = NULL;
    job_ = NULL;
    has_more_ = false; 
    have_status_ = false;
    selector_ = default_selector;
}

void JobQueueIterator::next(void)
{
    int ret;
    Dbt key, value;
    key.set_flags(0);
    value.set_flags(0);
    for (;;) {
        ret = cursor_->get(&key, &value, DB_NEXT); 
        if (ret == DB_NOTFOUND) {
            has_more_ = false;
            break;
        }
        // free(key.get_data());
        ByteArray a(value.get_data(), value.get_size());
        // free(value.get_data());
        job_ = new Job(a);
        if (have_status_ == false) {
            // only one query
            break;
        } else {
            if (selector_->match(job_) == true) {
                // query until found job right by selector
                break;
            } else {
                delete job_;
                job_ = NULL;
            }
        }
    }
}


JobQueueIterator::JobQueueIterator(DbTxn *tid, Dbc *cursor)
{
    has_more_ = true;
    tid_ = tid;
    cursor_ = cursor;
    have_status_ = false;
    job_ = NULL;
    selector_ = default_selector;
    next();
}

JobQueueIterator::JobQueueIterator(DbTxn *tid, Dbc *cursor, JobSelector *selector)
{
    has_more_ = true;
    tid_ = tid;
    cursor_ = cursor;
    have_status_ = true;
    job_ = NULL;
    selector_ = selector;
    next();
}

const JobQueueIterator &JobQueueIterator::operator++()
{
    delete job_;
    job_ = NULL;
    next();
    return *this;
}

const JobQueueIterator &JobQueueIterator::operator++(int)
{
    delete job_;
    job_ = NULL;
    next();
    return *this;
}

void JobQueueIterator::finish(void)
{
    if (job_ != NULL) {
        delete job_;
        job_ = NULL;
    }
    if (cursor_ != NULL) {
        cursor_->close();
        cursor_ = NULL;
    }
    if (tid_ != NULL) {
        tid_->commit(0);
        tid_ = NULL;
    }
}

void JobQueueIterator::refresh()
{
    // generate key
    void *buf = (void *)job_->getID().c_str();
    int size = job_->getID().size() + 1;
    Dbt key(buf, size);
    // generate data
    ByteArray &a = job_->serialize();
    Dbt data(a.data(), a.size());
    cursor_->put(&key, &data, DB_KEYFIRST);
}

void JobQueueIterator::remove()
{
    cursor_->del(0);
}

JobQueueIterator::~JobQueueIterator()
{
    finish();
}

void JobQueue::init(const std::string &dbroot, const std::string &store_name)
{
    env_ = NULL;
    db_ = NULL;
    env_ = new DbEnv(0); // Exception will occure
    // env_->open(dbroot.c_str(), DB_CREATE | DB_INIT_CDB | DB_INIT_MPOOL | DB_THREAD, 0644);
    env_->open(dbroot.c_str(), DB_CREATE | DB_INIT_MPOOL | DB_INIT_LOCK | DB_INIT_TXN | DB_RECOVER | DB_THREAD, 0644);
    // setup internal deadlock detection mechanizm
    env_->set_lk_detect(DB_LOCK_DEFAULT);
    db_ = new Db(env_, 0);
#if (DB_VERSION_MAJOR < 4) || (DB_VERSION_MAJOR == 4 && DB_VERSION_MINOR == 0)
    db_->open(store_name.c_str(), NULL, DB_BTREE, DB_CREATE | DB_AUTO_COMMIT | DB_THREAD, 0644);
#else
    db_->open(NULL, store_name.c_str(), NULL, DB_BTREE, DB_CREATE | DB_AUTO_COMMIT | DB_THREAD, 0644);
#endif
}

JobQueue::~JobQueue(void) 
{
    if (db_ != NULL) {
        db_->close(0);
        delete db_;
    }
    if (env_ != NULL) {
        env_->close(0);
        delete env_;
    }
}

void JobQueue::refresh(Job &j)
{
    DbTxn *tid = NULL;
    try {
        env_->txn_begin(NULL, &tid, 0);
    } catch (std::exception &e) {
        return;
    }
    try {
        // generate key
        void *buf = (void *)j.getID().c_str();
        int size = j.getID().size() + 1;
        Dbt key(buf, size);
        // generate data
        ByteArray &a = j.serialize();
        Dbt data(a.data(), a.size());
        db_->put(tid, &key, &data, 0);
        tid->commit(0);
    } catch (DbException &e) {
        tid->abort();
    }
}

Job *JobQueue::operator[](const std::string &id)
{
    DbTxn *tid = NULL;
    try {
        env_->txn_begin(NULL, &tid, 0);
    } catch (std::exception &e) {
        return NULL;
    }
    try {
        void *buf = (void *)id.c_str();
        int size = id.size() + 1;
        Dbt key(buf, size);
        Dbt data;
        data.set_flags(DB_DBT_MALLOC);
        if (db_->get(tid, &key, &data, 0) != DB_NOTFOUND) {
            ByteArray a(data.get_data(), data.get_size());
            free(data.get_data());
            Job *j = new Job(a);
            tid->commit(0);
            return j;
        } else {
            tid->commit(0);
            throw JobNotFoundException();
        }
    } catch (DbException &e) {
        tid->abort();
    }
}

void JobQueue::remove(Job &j)
{
    remove(j.getID());
}

void JobQueue::remove(const std::string &id)
{
    DbTxn *tid = NULL;
    try {
        env_->txn_begin(NULL, &tid, 0);
    } catch (std::exception &e) {
        return;
    }
    try {
        void *buf = (void *)id.c_str();
        int size = id.size() + 1;
        Dbt key(buf, size);
        db_->del(tid, &key, 0);
        tid->commit(0);
    } catch (DbException &e) {
        tid->abort();
    }
}

JobQueueIterator JobQueue::getAll(void) 
{
    Dbc *cursor;
    DbTxn *tid = NULL;
    try {
        env_->txn_begin(NULL, &tid, 0);
    } catch (std::exception &e) {
        return JobQueueIterator();
    }
    try {
        db_->cursor(tid, &cursor, 0);
        return JobQueueIterator(tid, cursor);
    } catch (DbException &e) {
        tid->abort();
        return JobQueueIterator();
    }
}

JobQueueIterator JobQueue::getAll(JobSelector *selector_) 
{
    Dbc *cursor;
    DbTxn *tid = NULL;
    try {
        env_->txn_begin(NULL, &tid, 0);
    } catch (std::exception &e) {
        return JobQueueIterator();
    }
    try {
        db_->cursor(tid, &cursor, 0);
        return JobQueueIterator(tid, cursor, selector_);
    } catch (DbException &e) {
        tid->abort();
        return JobQueueIterator();
    }
}

void JobQueue::sync(void)
{
    // db_->sync(0);
}

}
