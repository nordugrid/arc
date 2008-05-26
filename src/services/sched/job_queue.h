#ifndef __ARC__JOB_QUEUE_H__
#define __ARC__JOB_QUEUE_H__ 

#include <string>

#ifdef HAVE_DB_CXX_H
#include <db_cxx.h>
#endif

#ifdef HAVE_DB4_DB_CXX_H
#include <db4/db_cxx.h>
#endif

#ifdef HAVE_DB44_DB_CXX_H
#include <db44/db_cxx.h>
#endif

#include "job.h"

namespace Arc {

class JobNotFoundException: public std::exception
{
    virtual const char* what() const throw()
    {
        return "JobNotFound ";
    }
};

class JobQueueIterator
{
    friend class JobQueue;
    private:
        DbTxn *tid_;
        Dbc *cursor_;
        bool has_more_;
        Job *job_;
        bool have_status_;
        SchedJobStatus status_;
    protected:
        JobQueueIterator(DbTxn *tid_, Dbc *cursor);
        JobQueueIterator(DbTxn *tid_, Dbc *cursor, SchedJobStatus status);
        void next(void);
    public:
        JobQueueIterator();
        ~JobQueueIterator();
        bool hasMore(void) const { return has_more_; };
        Job *operator*() const { return job_; };
        const JobQueueIterator &operator++();
        const JobQueueIterator &operator++(int);
        void write_back(Job &);
        void finish(void);
};

class JobQueue 
{
    private:
        DbEnv *env_;
        Db *db_;
        DbTxn *tid_;
    public:
        JobQueue() { env_ = NULL; db_ = NULL; tid_ = NULL; };
        ~JobQueue();
        void init(const std::string &dbroot, const std::string &store_name);
        void refresh(Job &j);
        Job *operator[](const std::string &id);
        void remove(Job &job);
        void remove(const std::string &id);
        JobQueueIterator getAll(void);
        JobQueueIterator getAll(SchedJobStatus status);
};

} // namespace Arc

#endif //  __ARC__JOB_QUEUE_H__

