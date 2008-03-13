#ifndef __ARC_JOB_STATUS_H__
#define __ARC_JOB_STATUS_H__

#include <map>
#include <string>

namespace Paul
{

enum JobStatusLevel 
{
    ACCEPTING, 
    ACCEPTED, 
    PREPARING, 
    PREPARED, 
    SUBMITTING, 
    EXECUTING, 
    KILLING, 
    EXECUTED, 
    FINISHING, 
    FAILED, 
    HELD
};

class JobStatus
{
    private:
        JobStatusLevel level;
        std::map<int, std::string> str_map;
    public:
        JobStatus(void);
        JobStatus(JobStatusLevel l);
        void add_map(std::map<int, std::string> &m);
        JobStatusLevel get(void);
        operator std::string(void);
}; // job status

#if 0
/// XXX Factroy generates Job status: factory gets the string map

template <typename T, SM>
class JobStatus
{
    private:
        T level;
        std::map<T, std::string> str_map;
    public:
        JobStatus(void);
        JobStatus(std::map<T, std::string> const &);
        JobStatus(T const&);
        JobStatus(std::string &);
        T get(void);
        operator std::string(void) const;
}; // class JobStatus
#endif

}; // namespace Paul

#endif
