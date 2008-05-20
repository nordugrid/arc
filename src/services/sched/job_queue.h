#ifndef __ARC__JOB_QUEUE_H__
#define __ARC__JOB_QUEUE_H__ 

#include <db_cxx.h>
#include <string>
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
        Dbc *cursor_;
        bool has_more_;
        Job *job_;
    protected:
        JobQueueIterator(Dbc *cursor);
        void next(void);
    public:
        JobQueueIterator();
        ~JobQueueIterator();
        bool hasMore(void) const { return has_more_; };
        Job *operator*() const { return job_; };
        const JobQueueIterator &operator++();
        const JobQueueIterator &operator++(int);
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
};

} // namespace Arc

#endif //  __ARC__JOB_QUEUE_H__

