#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "job.h"

JobRequest::JobRequest(void) {
	reset();
}

JobRequest::~JobRequest() {
	for(std::list<JobRequest*>::iterator i = alternatives.begin();
	                                         i!=alternatives.end();) {
		JobRequest* j = *i;
		i=alternatives.erase(i);
		if(j) delete j;
	};

}

JobRequest::JobRequest(const JobRequest& j) {
	operator=(j);
}

std::ostream& operator<<(std::ostream& o,JobRequest& j) throw(JobRequestError) {
	std::string s;
	j.print(s);
	o<<s;
	return o;
}

bool JobRequest::print(std::string& s) throw(JobRequestError) {
	throw JobRequestError("Printing of job request is not implemented");
	return false;
}

JobRequest::InputFile::InputFile(const std::string& n,const std::string& s):name(n) {
	if(s.find(':') != std::string::npos) {
		//@ try {
			source=s;
		//@ } catch(URLError) { };
	} else {
		parameters=s;
	};
}

JobRequest::OutputFile::OutputFile(const std::string& n,const std::string& d):name(n) {
	if(d.length()) {
		//@ try {
			destination=d;
		//@ } catch(URLError) { };
	}
}

JobRequest::Notification::Notification(const std::string& f,const std::string& e):flags(f),email(e) {
} 

void JobRequest::reset(void) {
    job_name.resize(0);
    acl.resize(0);
    start_time=Arc::Time(-1);
    sstdin.resize(0);
    sstdout.resize(0);
    sstderr.resize(0);
    reruns=-1;
    lifetime=-1;
    gmlog.resize(0);
    credentialserver.resize(0);
    cluster.resize(0);
    architecture.resize(0);
    queue.resize(0);
    arguments.clear();
    runtime_environments.clear();
    loggers.clear();
    count=-1;
    memory=-1;
    disk=-1;
    cpu_time=-1;
    wall_time=-1;
    grid_time=-1;
    client_hostname.resize(0);
    client_software.resize(0);
    inputdata.clear();
    outputdata.clear();
    notifications.clear();
    middlewares.clear();
    executables.clear();
}

JobRequest& JobRequest::operator=(const JobRequest& j) throw (JobRequestError) {
    job_name=j.job_name;
    acl=j.acl;
	start_time=j.start_time;
    sstdin=j.sstdin;
    sstdout=j.sstdout;
    sstderr=j.sstderr;
    reruns=j.reruns;
    lifetime=j.lifetime;
    gmlog=j.gmlog;
	credentialserver=j.credentialserver;
	cluster=j.cluster;
	architecture=j.architecture;
	queue=j.queue;
	{
		std::list<std::string>::iterator i = arguments.begin();
		std::list<std::string>::const_iterator i_ = j.arguments.begin();
		for(;i_!=j.arguments.end();++i_,++i) {
			if(i == arguments.end()) i=arguments.insert(arguments.end(),"");
			if(i_->length()) *i=*i_;
		};
	};
    runtime_environments=j.runtime_environments;
    loggers.clear();
    count=j.count;
    memory=j.memory;
    disk=j.disk;
    cpu_time=j.cpu_time;
    wall_time=j.wall_time;
    grid_time=j.grid_time;
    client_hostname=j.client_hostname;
    client_software=j.client_software;
    inputdata=j.inputdata;
    outputdata=j.outputdata;
    notifications=j.notifications;
    middlewares=j.middlewares;
    executables=j.executables;
	alternatives.clear();
	for(std::list<JobRequest*>::const_iterator i = j.alternatives.begin();
	                                         i!=j.alternatives.end();++i) {
		alternatives.push_back(new JobRequest(*(*i)));
	};
	return (*this);
}

bool JobRequest::IsSimple(void) {
	return (alternatives.size() == 0);
}

void JobRequest::SplitToSimple(std::list<JobRequest>& jobs,int depth) throw(JobRequestError) {
	if(!(alternatives.size())) {
		jobs.push_back(*this);
		return;
	};
	for(std::list<JobRequest*>::const_iterator i = alternatives.begin();
	                                         i!=alternatives.end();++i) {
		std::list<JobRequest>::iterator j =
		                jobs.insert(jobs.end(),*this);
		j->merge(*(*i));
		if((!(j->IsSimple())) && (depth > 0)) {
			std::list<JobRequest> jobs_;
			j->SplitToSimple(jobs_,depth-1);
			jobs.splice(jobs.end(),jobs_,jobs_.begin(),jobs_.end());
			jobs.erase(j);
		};
	};
}

void JobRequest::merge(const JobRequest& j) {
	if(j.job_name.length()) job_name=j.job_name;
	if(j.arguments.size()) arguments=j.arguments;
	if(j.executables.size()) executables=j.executables;
	runtime_environments.insert(runtime_environments.end(),
	       j.runtime_environments.begin(),j.runtime_environments.end());
	if(j.middlewares.size()) middlewares=j.middlewares;
	if(j.acl.length()) acl=j.acl;
	if(j.start_time != Arc::Time(-1)) start_time=j.start_time;
	if(j.gmlog.length()) gmlog=j.gmlog;
	if(j.credentialserver.length()) credentialserver=j.credentialserver;
	if(j.architecture.length()) architecture=j.architecture;
	if(j.sstdin.length()) sstdin=j.sstdin;
	if(j.sstdout.length()) sstdout=j.sstdout;
	if(j.sstderr.length()) sstderr=j.sstderr;
	if(j.queue.length()) queue=j.queue;
	if(j.notifications.size()) notifications=j.notifications;
	if(j.inputdata.size()) inputdata=j.inputdata;
	if(j.outputdata.size()) outputdata=j.outputdata;
	if(j.lifetime >= 0) lifetime=j.lifetime;
	if(j.memory >= 0) memory=j.memory;
	if(j.disk >= 0) disk=j.disk;
	if(j.cpu_time >= 0) cpu_time=j.cpu_time;
	if(j.wall_time >= 0) wall_time=j.wall_time;
	if(j.grid_time >= 0) grid_time=j.grid_time;
	if(j.count >= 0) count=j.count;
	if(j.reruns >= 0) reruns=j.reruns;
	if(j.client_hostname.length()) client_hostname=j.client_hostname;
	if(j.client_software.length()) client_software=j.client_software;
	if(j.loggers.size()) loggers=j.loggers;
	alternatives.clear();
	for(std::list<JobRequest*>::const_iterator i = j.alternatives.begin();
	                                         i!=j.alternatives.end();++i) {
		alternatives.push_back(new JobRequest(*(*i)));
	};
}

