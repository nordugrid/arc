#ifndef __ARC__JOB_QUEUE_H__
#define __ARC__JOB_QUEUE_H__ 

#include <string>
#include <db_cxx.h>
#include "job.h"

namespace Arc {

class JobNotFoundException: public std::exception
{
    virtual const char* what() const throw()
    {
        return "JobNotFound ";
    }
};

class JobSelector
{
    public:
        JobSelector() {};
        virtual ~JobSelector() {};
        virtual bool match(Job *job) { return true; };
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
        JobSelector *selector_;
    protected:
        JobQueueIterator(DbTxn *tid_, Dbc *cursor);
        JobQueueIterator(DbTxn *tid_, Dbc *cursor, JobSelector *selector_);
        void next(void);
    public:
        JobQueueIterator();
        ~JobQueueIterator();
        bool hasMore(void) const { return has_more_; };
        Job *operator*() const { return job_; };
        const JobQueueIterator &operator++();
        const JobQueueIterator &operator++(int);
        void refresh(void);
        void remove(void);
        void finish(void);
};

class JobQueue 
{
    private:
        DbEnv *env_;
        Db *db_;
    public:
        JobQueue() { env_ = NULL; db_ = NULL; };
        ~JobQueue();
        void init(const std::string &dbroot, const std::string &store_name);
        void refresh(Job &j);
        Job *operator[](const std::string &id);
        void remove(Job &job);
        void remove(const std::string &id);
        JobQueueIterator getAll(void);
        JobQueueIterator getAll(JobSelector *selector_);
        void sync(void);
};

} // namespace Arc

#endif //  __ARC__JOB_QUEUE_H__

