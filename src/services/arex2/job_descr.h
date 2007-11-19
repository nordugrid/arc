#ifndef __AREX2_JOB_DESCR_H__
#define __AREX2_JOB_DESCR_H__ 

#include <string>
#include <arc/URL.h>
#include <arc/DateTime.h>
#include "rte.h"

namespace ARex2
{
/** Internal representation of Job described by JSDL */
class JobDescription 
{
    public:
        /** Class represents the one of the input file of the job */
		class InputFile {
			public:
				std::string name;
				std::string parameters;
				Arc::URL source;
				InputFile(const std::string& name, const std::string& source);
		};
        /** Class represents the one of the output file of the job */
		class OutputFile {
			public:
				std::string name;
				Arc::URL destination;
				OutputFile(const std::string& name, 
                           const std::string& destination);
		};
        /** Class represents notification requiest */
		class Notification {
			public:
				std::string flags;
				std::string email;
				Notification(const std::string& flags,
                             const std::string& email);
		};

	protected:
		std::string job_name;
		std::list<std::string> arguments;
		std::list<std::string> executables;
		std::list<RTE> rtes;
		std::list<RTE> middlewares;
		std::string architecture;
		std::string acl;
		Arc::Time start_time;
		std::string gmlog;
		std::list<std::string> loggers;
		std::string credentialserver;
		std::string cluster;
		std::string queue;
		std::string sstdin;
		std::string sstdout;
		std::string sstderr;
		std::list<Notification> notifications;
		long lifetime;
		std::list<InputFile> inputdata;
		std::list<OutputFile> outputdata;
		int memory;
		int disk;
		long cpu_time;
		long wall_time;
		long grid_time;
		int count;
		int reruns;
		std::string client_software;
		std::string client_hostname;
		// std::list<JobRequest*> alternatives;

#if 0
		virtual bool print(std::string& s) throw(JobRequestError);
		/** Merge job 'j' with this instance. Defined values in 'j'
		  * overwrite those in this. */
		void merge(const JobRequest& j);
		/** Set all attributes to default values - undefined */
        void reset(void);
#endif

	public:
		JobDescription(void);
		JobDescription(const JobDescription& j);
        virtual JobDescription& operator=(const JobDescription& j)
		virtual ~JobDescription(void);
		
        /**
		  * Interface methods to access stored values.
		  */
		std::string& JobName(void) { return job_name; };
        std::list<std::string>& Arguments(void) { return arguments; };
        std::list<std::string>& Executables(void) { return executables; };
        std::list<RTE>& RuntimeEnvironments(void) { return runtime_environments; };
        std::list<RTE>& Middlewares(void) { return middlewares; };
        std::string& ACL(void) { return acl; };
        std::string& GMLog(void) { return gmlog; };
        std::list<std::string>& Loggers(void) { return loggers; };
        std::string& CredentialServer(void) { return credentialserver; };
        std::string& Stdin(void) { return sstdin; };
        std::string& Stdout(void) { return sstdout; };
        std::string& Stderr(void) { return sstderr; };
        std::string& Queue(void) { return queue; };
        std::list<Notification>& Notifications(void) { return notifications; };
        long LifeTime(void) { return lifetime; };
        std::list<InputFile>& InputData(void) { return inputdata; };
        std::list<OutputFile>& OutputData(void) { return outputdata; };
        int Memory(void) { return memory; };
        long CPUTime(void) { return cpu_time; };
        long WallTime(void) { return wall_time; };
        long GridTime(void) { return grid_time; };
        int Count(void) { return count; };
        int Reruns(void) { return reruns; };
        std::string& ClientSoftware(void) { return client_software; };
        std::string& ClientHostname(void) { return client_hostname; };
		int Disk(void) { return disk; };

};
}; // namespace ARex2

#endif
