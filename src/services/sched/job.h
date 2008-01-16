#ifndef SCHED_JOB
#define SCHED_JOB

#include <string>
#include <list>

#include "../../../../hed/libs/common/XMLNode.h"


typedef enum {
  JOB_STATE_ACCEPTED  = 0,
  JOB_STATE_PREPARING = 1,
  JOB_STATE_SUBMITING = 2,
  JOB_STATE_INLRMS    = 3,
  JOB_STATE_FINISHING = 4,
  JOB_STATE_FINISHED  = 5,
  JOB_STATE_DELETED   = 6,
  JOB_STATE_CANCELING = 7,
  JOB_STATE_UNDEFINED = 8
} job_state_type;

class JobError : public ARCLibError {
    public:
        JobError(std::string message) : ARCLibError(message) {}
};

class Job {

	private:
		std::string job_name;
		std::list<std::string> arguments;
		std::string executable;
		std::string input_file;
		std::string output_file;
		std::string error_file;
        long memory_limit;
        long cpu_time_limit;
        int  total_cpu_count;
        job_state_type job_state;
        std::string job_id;
        std::string session_dir;

	public:
		Job(void);
		Job(const XMLNode& JSDL);
        virtual Job& operator=(const Job& j) throw(JobError);
		virtual ~Job(void);
		std::string& JobName(void) { return job_name; };
        std::list<std::string>& Arguments(void) { return arguments; };
        std::string& Executable(void) { return executable; };
        std::string& Input(void) { return input_file; };
        std::string& Output(void) { return output_file; };
        std::string& Error(void) { return error_file; };
        int MemoryLimit(void) { return memory; };
        long CPUTimeLimit(void) { return cpu_time; };
        long WallTimeLimit(void) { return wall_time; };
        int CPUCount(void) { return count; };
};

#endif // SCHED_JOB

