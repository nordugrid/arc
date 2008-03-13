#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "job_xrsl.h"
#include <arc/StringConv.h>
#include <arc/DateTime.h>
#include <arc/URL.h>

class URLModifier: public Arc::URL {
	public:
		void AddOption(const std::string& name,const std::string& value) {
			std::map<std::string, std::string>::value_type option(name,value);
			urloptions.insert(option);
		};
		void ModifyOption(const std::string& name,const std::string& value) {
			urloptions[name]=value;
		};
		void Host(const std::string& s) {
			host=s;
		};
		void Path(const std::string& s) {
			path=s;
		};
};


long divide_up(long x,long y) {
  return ((x+y-1)/y);
}

static void add_attribute(const std::string& name,const std::string& value,Xrsl& xrsl) throw(XrslError) {
	if(value.length()) {
		xrsl.AddRelation(XrslRelation(name,operator_eq,value));
	};
}

static void add_attribute(const std::string& name,long value,Xrsl& xrsl) throw(XrslError) {
	if(value>=0) {
		xrsl.AddRelation(XrslRelation(name,operator_eq,Arc::tostring<long>(value)));
	};
}

static void add_attribute(const std::string& name,const Arc::Time& value,Xrsl& xrsl) throw(XrslError) {
	if(value != Arc::Time(-1)) {
		xrsl.AddRelation(XrslRelation(name,operator_eq,value.str()));
	};
}

static void add_attribute(const std::string& name,const std::list<std::string>& values,Xrsl& xrsl) throw(XrslError) {
	if(values.size()) {
		xrsl.AddRelation(XrslRelation(name,operator_eq,values));
	};
}

static void add_attribute_period(const std::string& name,long value,Xrsl& xrsl) throw(XrslError) {
	if(value>=0) {
		xrsl.AddRelation(XrslRelation(name,operator_eq,Arc::Period(value)));
	};
}

static void add_attributes(const std::string& name,const std::list<std::string>& values,Xrsl& xrsl) throw(XrslError) {
	for(std::list<std::string>::const_iterator i = values.begin();
	                                           i!=values.end();++i) {
		add_attribute(name,*i,xrsl);
	};
}

static void get_attribute(const std::string& name,std::string& value,Xrsl& xrsl) {
    try {
        value=(xrsl.GetRelation(name)).GetSingleValue();
    } catch (XrslError) { };
}

static void get_attribute(const std::string& name,int& value,Xrsl& xrsl) {
    try {
        value=Arc::stringto<int>((xrsl.GetRelation(name)).GetSingleValue());
    } catch (XrslError) {
    };
}

/*
static void get_attribute(const std::string& name,long& value,Xrsl& xrsl) {
    try {
        value=Arc::stringto<long>((xrsl.GetRelation(name)).GetSingleValue());
    } catch (XrslError) {
    };
}
*/

static void get_attribute(const std::string& name,Arc::Time& value,Xrsl& xrsl) {
    try {
        value=Arc::Time((xrsl.GetRelation(name)).GetSingleValue());
    } catch (XrslError) {
    };
}

/*
static void get_attribute_seconds(const std::string& name,long value,Xrsl& xrsl) {
	try {
		std::string v=(xrsl.GetRelation(name)).GetSingleValue();
		value=Arc::Period(v,Arc::PeriodSeconds).GetPeriod();
	} catch (XrslError) {
	};
}
*/

static void get_attribute_minutes(const std::string& name,long value,Xrsl& xrsl) {
	try {
		std::string v=(xrsl.GetRelation(name)).GetSingleValue();
		value=Arc::Period(v,Arc::PeriodMinutes).GetPeriod();
	} catch (XrslError) {
        };
}

static void get_attribute_days(const std::string& name,long value,Xrsl& xrsl) {
	try {
		std::string v=(xrsl.GetRelation(name)).GetSingleValue();
		value=Arc::Period(v,Arc::PeriodDays).GetPeriod();
	} catch (XrslError) {
        };
}

static std::string notification(const JobRequest::Notification v) {
	return v.flags+" "+v.email;
}

static std::string notification(const std::list<JobRequest::Notification> l) {
	std::string n;
	for(std::list<JobRequest::Notification>::const_iterator i = l.begin();
	                                                       i!=l.end();++i) {
		if(n.length()) n+=" ";
		n+=notification(*i);
	};
	return n;
}

static std::list<JobRequest::Notification> notification(const std::string& s) {
	std::list<JobRequest::Notification> n;
	std::string::size_type start = 0;
	std::string::size_type end = 0;
	std::string flags("");
	for(;;) {
		start=s.find_first_not_of(' ',end);
		if(start == std::string::npos) break;
		end=s.find(' ',start);
		if(end == std::string::npos) end=s.length();
		std::string::size_type p = s.find('@',start);
		if((p == std::string::npos) || (p >= end)) {
			flags=s.substr(start,end-start);
			continue;
		};
		if(flags.length() == 0) continue;
		n.push_back(JobRequest::Notification(flags,s.substr(start,end-start)));
	};
	return n;
}


bool JobRequestXRSL::set(const char* s) throw(JobRequestError) {
	reset();
	if(xrsl_) delete xrsl_;
	try {
		xrsl_=new Xrsl(std::string(s));
	} catch (XrslError e) {
		xrsl_=NULL;
		return false;
	};
    return set(*xrsl_);
}

bool JobRequestXRSL::set(Xrsl& xrsl) throw(JobRequestError) {
	// Simple attributes
	get_attribute("jobname",job_name,xrsl);
	get_attribute("acl",acl,xrsl);
	get_attribute("starttime",start_time,xrsl);
	get_attribute("stdin",sstdin,xrsl);
	get_attribute("stdout",sstdout,xrsl);
	get_attribute("stderr",sstderr,xrsl);
	get_attribute("gmlog",gmlog,xrsl);
	get_attribute("credentialserver",credentialserver,xrsl);
	get_attribute("cluster",cluster,xrsl);
	get_attribute("architecture",architecture,xrsl);
	get_attribute("queue",queue,xrsl);
	{
		std::string n;
		get_attribute("notify",n,xrsl);
		notifications=notification(n);
	};
	/*client_software*/
	get_attribute("hostname",client_hostname,xrsl);
	get_attribute_days("lifetime",lifetime,xrsl);
	get_attribute("memory",memory,xrsl);
	get_attribute("disk",disk,xrsl);
	get_attribute("count",count,xrsl);
	get_attribute("rerun",reruns,xrsl);
	get_attribute_minutes("cputime",cpu_time,xrsl);
	get_attribute_minutes("walltime",wall_time,xrsl);
	get_attribute_minutes("gridtime",grid_time,xrsl);

	// Complex attributes
	try {
		arguments.push_back((xrsl.GetRelation("executable")).GetSingleValue());
	} catch (XrslError) { arguments.push_back(""); };
	try {
		std::list<std::string> args =
		              (xrsl.GetRelation("arguments")).GetListValue();
		arguments.insert(arguments.end(),args.begin(),args.end());
	} catch (XrslError) { };
	try {
        std::list<XrslRelation> rels =
		            xrsl.GetAllRelations("runtimeenvironment");
		for(std::list<XrslRelation>::iterator i = rels.begin();
		                                        i!=rels.end();++i) {
			try {
				runtime_environments.push_back(
				            RuntimeEnvironment(i->GetSingleValue()));
			} catch (XrslError) { };
		};
	} catch (XrslError) { };
	try {
		std::list<XrslRelation> rels =
		            xrsl.GetAllRelations("middleware");
		for(std::list<XrslRelation>::iterator i = rels.begin();
		                                        i!=rels.end();++i) {
			try {
				middlewares.push_back(RuntimeEnvironment(i->GetSingleValue()));
			} catch (XrslError) { };
		};
	} catch (XrslError) { };
	try {
		std::list<XrslRelation> rels =
		            xrsl.GetAllRelations("jobreport");
		for(std::list<XrslRelation>::iterator i = rels.begin();
		                                        i!=rels.end();++i) {
			try {
				loggers.push_back(i->GetSingleValue());
			} catch (XrslError) { };
		};
	} catch (XrslError) { };
	try {
		std::list<std::list<std::string> > dfiles =
		         (xrsl.GetRelation("inputfiles")).GetDoubleListValue();
		for(std::list<std::list<std::string> >::iterator f = dfiles.begin();
		                                              f!=dfiles.end();++f) {
			std::list<std::string>::iterator i = f->begin();
			if(i == f->end()) continue;
			std::string name = *i; ++i;
			if(i != f->end()) {
				inputdata.push_back(InputFile(name,*i));
			} else {
				inputdata.push_back(InputFile(name,""));
			};
		};
	} catch (XrslError) { };
	try {
		std::list<std::list<std::string> > dfiles =
		         (xrsl.GetRelation("outputfiles")).GetDoubleListValue();
		for(std::list<std::list<std::string> >::iterator f = dfiles.begin();
		                                              f!=dfiles.end();++f) {
			std::list<std::string>::iterator i = f->begin();
			if(i == f->end()) continue;
			std::string name = *i; ++i;
			if(i != f->end()) {
				outputdata.push_back(OutputFile(name,*i));
			} else {
				outputdata.push_back(OutputFile(name,""));
			};
		};
	} catch (XrslError) { };
	try {
		executables=(xrsl.GetRelation("executables")).GetListValue();
	} catch (XrslError) { };

	try {
	    std::list<Xrsl> xrsls = xrsl.SplitOrRelation();
		if(xrsls.size() > 1) {
			for(std::list<Xrsl>::iterator i = xrsls.begin();
			                                i!=xrsls.end();++i) {
				JobRequest* jr = new JobRequest;
				((JobRequestXRSL*)jr)->set(*i);
				alternatives.push_back(jr);
			};
		};
	} catch (XrslError) { };

	// Indirect attributes
	std::string value;
	value=""; get_attribute("cache",value,xrsl);
	if(value.length()) {
		for(std::list<InputFile>::iterator i = inputdata.begin();
		                                   i!=inputdata.end();++i) {
			if(i->source.str().length())
				((URLModifier*)(&(i->source)))->AddOption("cache",value);
		};
	};
	value=""; get_attribute("join",value,xrsl);
	if(value.length()) {
		if(value == "yes") {
			if(sstdout.length()) sstderr=sstdout;
			else if(sstderr.length()) sstdout=sstderr;
		};
	};
	value=""; get_attribute("ftpthreads",value,xrsl);
	if(value.length()) {
		for(std::list<InputFile>::iterator i = inputdata.begin();
		                                   i!=inputdata.end();++i) {
			if(i->source.str().length())
				((URLModifier*)(&(i->source)))->AddOption("threads",value);
		};
		for(std::list<OutputFile>::iterator i = outputdata.begin();
		                                   i!=outputdata.end();++i) {
			if(i->destination.str().length())
				((URLModifier*)(&(i->destination)))->AddOption("threads",value);
		};
	};
	value=""; get_attribute("replicacollection",value,xrsl);
	if(value.length()) {
		Arc::URL rc_url(value);
		if(rc_url) {
			if(rc_url.Protocol() == "ldap") {
				for(std::list<InputFile>::iterator i = inputdata.begin();
		        		                           i!=inputdata.end();++i) {
					if(i->source.Host().length() == 0) {
						((URLModifier*)(&(i->source)))->Host(rc_url.Host());
						((URLModifier*)(&(i->source)))->Path(
							rc_url.Host()+i->source.Path());
					};
				};
			};
		};
	};
	return true;
}

bool JobRequestXRSL::print(std::string& s) throw(JobRequestError) {
	if(!xrsl_) return false;
	s=xrsl_->str();
	return true;
}

JobRequestXRSL::JobRequestXRSL(const JobRequest& j,Type type) throw(JobRequestError):xrsl_(NULL),type_(type) {
  operator=(j);
}

JobRequestXRSL::JobRequestXRSL(const char* s,Type type) throw(JobRequestError):xrsl_(NULL),type_(type) {
	if(!set(s)) throw JobRequestError("Can't parse xRSL");
}

JobRequestXRSL::JobRequestXRSL(const std::string& s,Type type) throw(JobRequestError):xrsl_(NULL),type_(type) {
	if(!set(s.c_str())) throw JobRequestError("Can't parse xRSL");
}

JobRequestXRSL::JobRequestXRSL(std::istream& i,Type type) throw(JobRequestError):xrsl_(NULL),type_(type) {
    char buf[256];
    std::string s;
    for(;!i.eof();) {
        i.get(buf,sizeof(buf),0);
        if(i.fail()) throw JobRequestError("Failed to read input stream of job request");
        if(buf[0] == 0) break;
        s.append(buf);
    };
    if(!set(s.c_str())) throw JobRequestError("Failed to parse job request");
}

JobRequestXRSL::~JobRequestXRSL(void) {
	if(xrsl_) delete xrsl_;
}


JobRequest& JobRequestXRSL::operator=(const JobRequest& j) throw (JobRequestError) {
	JobRequest::operator=(j);
	// Make xRSL
	if(xrsl_) delete xrsl_;
	xrsl_=new Xrsl;
	if(!xrsl_) return (*this);
	set_xrsl(*xrsl_);
	return (*this);
}

bool JobRequestXRSL::set_xrsl(Xrsl& xrsl) throw (JobRequestError) {
	try {
		add_attribute("jobname",job_name,xrsl);
		if(arguments.size() > 0) {
			xrsl.AddRelation(
			      XrslRelation("executable",operator_eq,*(arguments.begin())));
		};
		if(arguments.size() > 1) {
			std::list<std::string> args(++(arguments.begin()),arguments.end());
            xrsl.AddRelation(XrslRelation("arguments",operator_eq,args));
		};
		add_attribute("stdin", sstdin, xrsl);
		add_attribute("stdout",sstdout,xrsl);
		add_attribute("stderr",sstderr,xrsl);
		add_attribute("notify",notification(notifications),xrsl);
		add_attribute("queue",queue,xrsl);
		add_attribute("acl",acl,xrsl);
		add_attribute("gmlog",gmlog,xrsl);
		add_attribute("credentialserver",credentialserver,xrsl);
		add_attribute("cluster",cluster,xrsl);
		add_attribute("architecture",architecture,xrsl);
		add_attribute("memory",memory,xrsl);
		add_attribute("disk",disk,xrsl);
		switch(type_) {
			case UserFriendly:
                start_time.SetFormat(Arc::UserTime);
		        add_attribute("starttime",start_time,xrsl);
				add_attribute_period("cputime",cpu_time,xrsl);
				add_attribute_period("walltime",wall_time,xrsl);
				add_attribute_period("gridtime",grid_time,xrsl);
				add_attribute_period("lifetime",lifetime,xrsl);
				break;
			default: // safe way
                start_time.SetFormat(Arc::MDSTime);
		        add_attribute("starttime",start_time,xrsl);
				add_attribute("cputime",divide_up(cpu_time,60),xrsl);
				add_attribute("walltime",divide_up(wall_time,60),xrsl);
				add_attribute("gridtime",divide_up(grid_time,60),xrsl);
				add_attribute("lifetime",divide_up(lifetime,60*60*24),xrsl);
				break;
		}
		add_attribute("count",count,xrsl);
		add_attribute("rerun",reruns,xrsl);
		for(std::list<RuntimeEnvironment>::const_iterator i =
		                 middlewares.begin();i!=middlewares.end();++i) {
			add_attribute("middleware",i->str(),xrsl);
		};
		add_attributes("jobreport",loggers,xrsl);
        add_attribute("executables",executables,xrsl);
        add_attribute("hostname",client_hostname,xrsl);
        /*std::string client_software;*/
		for(std::list<RuntimeEnvironment>::iterator i =
		      runtime_environments.begin();i!=runtime_environments.end();++i) {
			add_attribute("runtimeenvironment",i->str(),xrsl);
		};
		if(inputdata.size()) {
			std::list<std::list<std::string> > dfiles;
			for(std::list<InputFile>::iterator i = inputdata.begin();
			                                    i!=inputdata.end();++i) {
				std::list<std::string> dfile;
				dfile.push_back(i->name);
				std::string src = i->source.str();
				if(src.length()) {
					dfile.push_back(src);
				} else {
					dfile.push_back(i->parameters);
				};
				dfiles.push_back(dfile);
			};
			xrsl.AddRelation(XrslRelation("inputfiles",operator_eq,dfiles));
		};
		if(outputdata.size()) {
			std::list<std::list<std::string> > dfiles;
			for(std::list<OutputFile>::iterator i = outputdata.begin();
			                                    i!=outputdata.end();++i) {
				std::list<std::string> dfile;
				dfile.push_back(i->name);
				dfile.push_back(i->destination.str());
				dfiles.push_back(dfile);
			};
			xrsl.AddRelation(XrslRelation("outputfiles",operator_eq,dfiles));
		};
		Xrsl xs(operator_or);
		int xs_n = 0;
		for(std::list<JobRequest*>::iterator i = alternatives.begin();
		                                     i!=alternatives.end();++i) {
			if(!(*i)) continue;
			Xrsl x;
			((JobRequestXRSL*)(*i))->set_xrsl(x);
			xs.AddXrsl(x); ++xs_n;
		};
		if(xs_n) xrsl.AddXrsl(xs);
	} catch (XrslError) { };
	return true;
}

