#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/ByteArray.h>
#include "job_queue.h"

namespace Arc
{

JobQueueIterator::JobQueueIterator()
{
    tid_ = NULL;
    cursor_ = NULL;
    job_ = NULL;
    has_more_ = false; 
    have_status_ = false;
    status_ = JOB_STATUS_SCHED_UNKNOWN;
}

void JobQueueIterator::next(void)
{
    int ret;
    Dbt key, value;
    key.set_flags(DB_DBT_MALLOC);
    value.set_flags(DB_DBT_MALLOC);
    for (;;) {
        ret = cursor_->get(&key, &value, DB_NEXT); 
        if (ret == DB_NOTFOUND) {
            has_more_ = false;
            break;
        }
        free(key.get_data());
        ByteArray a(value.get_data(), value.get_size());
        free(value.get_data());
        job_ = new Job(a);
        if (have_status_ == false) {
            // only one query
            break;
        } else {
            if (job_->getStatus() == status_) {
                // query until found job with certain status
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
    status_ = JOB_STATUS_SCHED_UNKNOWN;
    job_ = NULL;
    next();
}

JobQueueIterator::JobQueueIterator(DbTxn *tid, Dbc *cursor, SchedJobStatus status)
{
    has_more_ = true;
    tid_ = tid;
    cursor_ = cursor;
    have_status_ = true;
    status_ = status;
    job_ = NULL;
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

void JobQueueIterator::write_back(Job &j)
{
    // generate key
    void *buf = (void *)j.getID().c_str();
    int size = j.getID().size() + 1;
    Dbt key(buf, size);
    // generate data
    ByteArray &a = j.serialize();
    Dbt data(a.data(), a.size());
    cursor_->put(&key, &data, DB_KEYFIRST);
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
    env_->open(dbroot.c_str(), DB_CREATE | DB_INIT_LOCK, 0644);
    db_ = new Db(env_, 0);
#if (DB_VERSION_MAJOR < 4) || (DB_VERSION_MAJOR == 4 && DB_VERSION_MINOR == 0)
    db_->open(store_name.c_str(), NULL, DB_BTREE, DB_CREATE, 0644);
#else
    // db_->open(NULL, store_name.c_str(), NULL, DB_BTREE, DB_CREATE | DB_THREAD, 0644);
    // db_->open(NULL, store_name.c_str(), NULL, DB_BTREE, DB_CREATE | DB_AUTO_COMMIT | DB_THREAD, 0644);
    // db_->open(NULL, store_name.c_str(), NULL, DB_BTREE, DB_CREATE | DB_THREAD, 0644);
    db_->open(NULL, store_name.c_str(), NULL, DB_BTREE, DB_CREATE, 0644);
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
    // generate key
    void *buf = (void *)j.getID().c_str();
    int size = j.getID().size() + 1;
    Dbt key(buf, size);
    // generate data
    ByteArray &a = j.serialize();
    Dbt data(a.data(), a.size());
    db_->put(NULL, &key, &data, 0);
}

Job *JobQueue::operator[](const std::string &id)
{
    void *buf = (void *)id.c_str();
    int size = id.size() + 1;
    Dbt key(buf, size);
    Dbt data;
    data.set_flags(DB_DBT_MALLOC);
    if (db_->get(NULL, &key, &data, 0) != DB_NOTFOUND) {
        ByteArray a(data.get_data(), data.get_size());
        free(data.get_data());
        Job *j = new Job(a);
        return j;
    } else {
        throw JobNotFoundException();
    }
}

void JobQueue::remove(Job &)
{
    // XXX NOP
}

JobQueueIterator JobQueue::getAll(void) 
{
    Dbc *cursor;
    DbTxn *tid = NULL;
    try {
//        env_->txn_begin(NULL, &tid, 0);
        db_->cursor(tid, &cursor, 0);
        return JobQueueIterator(tid, cursor);
    } catch (DbException &e) {
std::cout << "uff" << std::endl;
        tid->abort();
        return JobQueueIterator();
    }
}

JobQueueIterator JobQueue::getAll(SchedJobStatus status) 
{
    Dbc *cursor;
    DbTxn *tid = NULL;
    try {
//        env_->txn_begin(NULL, &tid, 0);
        db_->cursor(tid, &cursor, 0);
        return JobQueueIterator(tid, cursor, status);
    } catch (DbException &e) {
        tid->abort();
        return JobQueueIterator();
    }
}

}
