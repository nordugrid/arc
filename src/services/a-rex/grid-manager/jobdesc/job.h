/**
 */
#ifndef ARCLIB_JOB
#define ARCLIB_JOB

#include <string>
#include <list>
#include <iostream>

#include "runtimeenvironment.h"
#include <arc/URL.h>
#include <arc/DateTime.h>


/** Exception class thrown in case of errors with the JobRequest class. */
class JobRequestError : public ARCLibError {
    public:
        /** Standard exception class constructor. */
        JobRequestError(std::string message) : ARCLibError(message) {}
};

/**
 * Class to represent the request for computational job.
 * This class stores job request in parsed way.
 * To parse/generate job request in various languages, and also
 * to manipulate job's parameters derived classes must be used.
 */
class JobRequest;

/**
 * Dump job request to stream. Generic implementation simply calls 
 *  method JobRequest::print .
 */
std::ostream& operator<<(std::ostream& o,JobRequest& j) throw(JobRequestError);

class JobRequest {
	friend std::ostream& operator<<(std::ostream& o,JobRequest& j) throw(JobRequestError);
	public:
		class InputFile {
			public:
				std::string name;
				std::string parameters;
				Arc::URL source;
				InputFile(const std::string& name,const std::string& source);
		};
		class OutputFile {
			public:
				std::string name;
				Arc::URL destination;
				OutputFile(const std::string& name,const std::string& destination);
		};
		class Notification {
			public:
				std::string flags;
				std::string email;
				Notification(const std::string& flags,const std::string& email);
		};
	protected:
		std::string job_name;
		std::list<std::string> arguments;
		std::list<std::string> executables;
		std::list<RuntimeEnvironment> runtime_environments;
		std::list<RuntimeEnvironment> middlewares;
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
		std::list<JobRequest*> alternatives;

		virtual bool print(std::string& s) throw(JobRequestError);
		/** Merge job 'j' with this instance. Defined values in 'j'
		  * overwrite those in this. */
		void merge(const JobRequest& j);
		/** Set all attributes to default values - undefined */
        void reset(void);

	public:
		JobRequest(void);
		JobRequest(const JobRequest& j);
        virtual JobRequest& operator=(const JobRequest& j)
		                                   throw(JobRequestError);
		virtual ~JobRequest(void);
		/** Returns true if job has no sub-jobs */
		virtual bool IsSimple(void);
		/** Generate all possible plain jobs from complex job */
		virtual void SplitToSimple(std::list<JobRequest>& jobs,int depth = -1)
		                                   throw(JobRequestError);
		/**
		  * Interface methods to access stored values.
		  */
		std::string& JobName(void) { return job_name; };
        std::list<std::string>& Arguments(void) { return arguments; };
        std::list<std::string>& Executables(void) { return executables; };
        std::list<RuntimeEnvironment>& RuntimeEnvironments(void) { return runtime_environments; };
        std::list<RuntimeEnvironment>& Middlewares(void) { return middlewares; };
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

#endif // ARCLIB_JOB

