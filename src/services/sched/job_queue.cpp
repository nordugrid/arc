#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/ByteArray.h>
#include "job_queue.h"

namespace Arc
{

JobQueueIterator::JobQueueIterator()
{
    cursor_ = NULL;
    job_ = NULL;
    has_more_ = false; 
}

void JobQueueIterator::next(void)
{
    int ret;
    Dbt key, value;
    key.set_flags(DB_DBT_MALLOC);
    value.set_flags(DB_DBT_MALLOC);
    ret = cursor_->get(&key, &value, DB_NEXT); 
    if (ret == DB_NOTFOUND) {
        has_more_ = false;
    }
    free(key.get_data());
    ByteArray a(value.get_data(), value.get_size());
    free(value.get_data());
    job_ = new Job(a);
}

JobQueueIterator::JobQueueIterator(Dbc *cursor)
{
    has_more_ = true;
    cursor_ = cursor;
    next();
}

const JobQueueIterator &JobQueueIterator::operator++()
{
    delete job_;
    next();
    return *this;
}

const JobQueueIterator &JobQueueIterator::operator++(int)
{
    delete job_;
    next();
    return *this;
}

JobQueueIterator::~JobQueueIterator()
{
    if (job_ != NULL) {
        delete job_;
    }
    if (cursor_ != NULL) {
        cursor_->close();
    }
}

void JobQueue::init(const std::string &dbroot, const std::string &store_name)
{
    env_ = NULL;
    db_ = NULL;
    env_ = new DbEnv(0); // Exception will occure
    env_->open(dbroot.c_str(), DB_CREATE | DB_INIT_CDB | DB_INIT_MPOOL | DB_THREAD /*|  DB_INIT_TXN*/, 0644);
    db_ = new Db(env_, 0);
    db_->open(NULL, store_name.c_str(), NULL, DB_BTREE, DB_CREATE | DB_THREAD, 0644);
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

void JobQueue::remove(Job &j)
{
    // XXX NOP
}

JobQueueIterator JobQueue::getAll(void) 
{
    Dbc *cursor;
    try {
        db_->cursor(NULL, &cursor, 0);
        return JobQueueIterator(cursor);
    } catch (DbException &e) {
        return JobQueueIterator();
    }
}

}
