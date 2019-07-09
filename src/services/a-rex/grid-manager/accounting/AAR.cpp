#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <pwd.h>

#include <arc/DateTime.h>
#include <arc/FileUtils.h>
#include <arc/StringConv.h>

#include "../jobs/GMJob.h"
#include "../conf/GMConfig.h"
#include "../files/ControlFileHandling.h"

#include "AAR.h"

namespace ARex {
    const char * const sfx_diag        = ".diag";
    const char * const sfx_statistics  = ".statistics";

    Arc::Logger AAR::logger(Arc::Logger::getRootLogger(), "AAR");

    static void extract_integer(std::string& s,std::string::size_type n = 0) {
        for(;n<s.length();n++) {
            if(isdigit(s[n])) continue;
            s.resize(n); 
            break;
        }
        return;
    }

    static bool string_to_number(std::string& s, unsigned long long int& n) {
        extract_integer(s);
        if(s.length() == 0) return false;
        if(!Arc::stringto(s,n)) return false;
        return true;
    }

    static std::string get_file_owner(const std::string& fname) {
        struct stat st;
        if (Arc::FileStat(fname, &st, false)) {
            struct passwd pw_;
            struct passwd *pw;
            char buf[BUFSIZ];
            getpwuid_r(st.st_uid,&pw_,buf,BUFSIZ,&pw);
            if (pw != NULL && pw->pw_name) {
                return std::string(pw->pw_name);
            }
        }
        return "";
    }

    bool AAR::FetchJobData(const GMJob &job,const GMConfig& config) {
        // jobid
        jobid = job.get_id();

        /* analyze job.ID.local and store relevant information */
        JobLocalDescription local;
        if (!job_local_read_file(job.get_id(), config, local)) return false;

        // endpoint
        if (local.headnode.empty() || local.interface.empty()) {
            logger.msg(Arc::ERROR, "Cannot find information abouto job submission endpoint");
            return false;
        }
        endpoint = {local.interface, local.headnode};
        // *localid
        if (!local.localid.empty()) localid = local.localid;
        // ?queue
        if (!local.queue.empty()) queue = local.queue;
        // userdn
        if (!local.DN.empty()) userdn = local.DN;
        // VOMS info
        if (!local.voms.empty()) {
            // wlcgwo
            wlcgvo = local.voms.front();
            if (wlcgvo.at(0) = '/') wlcgvo.erase(0,1);
            // authtokenattrs
            for(std::list<std::string>::const_iterator it=local.voms.begin(); it != local.voms.end(); ++it) {
                authtokenattrs.push_back(aar_authtoken_t("vomsfqan", (*it)));
            }
        }
        // submittime
        submittime = local.starttime;
        // add submit time to event log on ACCEPTED
        if (job.get_state() == JOB_STATE_ACCEPTED) {
            aar_jobevent_t startevent(job.get_state_name(), local.starttime);
            jobevents.push_back(startevent);
        }
        aar_jobevent_t startevent(job.get_state_name(), Arc::Time());

        // extra info
        if (!local.jobname.empty()) extrainfo.insert(
            std::pair <std::string, std::string>("jobname", local.jobname)
        );
        if (!local.lrms.empty()) extrainfo.insert(
            std::pair <std::string, std::string>("lrms", local.lrms)
        );
        if (!local.clientname.empty()) extrainfo.insert(
            std::pair <std::string, std::string>("clienthost", local.clientname)
        );
        for (std::list<std::string>::const_iterator it = local.projectnames.begin();
            it != local.projectnames.end(); ++it) {
            extrainfo.insert(
                std::pair <std::string, std::string>("projectname", (*it))
            );
        }

        if (job.get_state() == JOB_STATE_ACCEPTED) {
            status = "accepted";
            // nothing from .diag and .statistics is relevant for just ACCEPTED jobs
            // so we can stop processing here
            return true;
        }

        // job completion status and endtime for FINISHED
        if (job.get_state() == JOB_STATE_FINISHED) {
            status = "completed";
            // end time
            time_t t = job_state_time(job.get_id(),config);
            if (t == 0) t=::time(NULL);
            endtime = Arc::Time(t);
            // end event
            aar_jobevent_t endevent(job.get_state_name(), endtime);
            jobevents.push_back(endevent);
            // failure
            if (job_failed_mark_check(job.get_id(),config)) {
                status = "failed";
            }
        }

        /* 
         * analyze job.ID.diag and store relevant information 
         */
        std::string fname_src = config.ControlDir() + G_DIR_SEPARATOR_S + "job." + job.get_id() + sfx_diag;
        std::list<std::string> diag_data;
        // nodenames used for node/cpus couting as well as extra info
        std::list<std::string> nodenames;
        // there are different memory metrics are avaiable from different sources
        // prefered is MAX memory, but use an AVARAGE metrics as fallback
        unsigned long long int mem_avg_total = 0;
        unsigned long long int mem_max_total = 0;
        unsigned long long int  mem_avg_resident = 0;
        unsigned long long int  mem_max_resident = 0;
        if (Arc::FileRead(fname_src, diag_data)) {
            for (std::list<std::string>::iterator line = diag_data.begin(); line != diag_data.end(); ++line) {
                // parse key=value lowercasing all keys
                std::string::size_type p = line->find('=');
                if (p == std::string::npos) continue;
                std::string key(Arc::lower(line->substr(0, p)));
                std::string value(line->substr(p+1));
                if (key.empty()) continue;
                // process keys
                if (key == "nodename") {
                    nodenames.push_back(value);
                } else if (key == "processors") {
                    unsigned long long int n;
                    if (string_to_number(value,n)) cpucount = (unsigned int) n;
                } else if (key == "exitcode") {
                    unsigned long long int n;
                    if (string_to_number(value,n)) exitcode = (unsigned int) n;
                } else if (key == "walltime" ) {
                    unsigned long long int n;
                    if (string_to_number(value,n)) usedwalltime = n;
                } else if (key == "kerneltime" ) {
                    unsigned long long int n;
                    if (string_to_number(value,n)) usedcpukerneltime = n;
                } else if (key == "usertime" ) {
                    unsigned long long int n;
                    if (string_to_number(value,n)) usedcpuusertime = n;
                } else if (key == "maxresidentmemory" ) {
                    unsigned long long int n;
                    if (string_to_number(value,n)) mem_max_resident = n;
                } else if (key == "averageresidentmemory" ) {
                    unsigned long long int n;
                    if (string_to_number(value,n)) mem_avg_resident = n;
                } else if (key == "maxtotalmemory" ) {
                    unsigned long long int n;
                    if (string_to_number(value,n)) mem_max_total = n;
                } else if (key == "averagetotalmemory" ) {
                    unsigned long long int n;
                    if (string_to_number(value,n)) mem_avg_total = n;
                } else if (key == "usedscratch" ) {
                    unsigned long long int n;
                    if (string_to_number(value,n)) usedscratch = n;
                } else if (key == "runtimeenvironments") {
                    // rtes are splited by semicolon
                    Arc::tokenize(value, rtes, ";");
                } else if (key == "lrmsstarttime") {
                    aar_jobevent_t lrmsevent("LRMSSTART", Arc::Time(value));
                    jobevents.push_back(lrmsevent);
                } else if (key == "lrmsendtime") {
                    aar_jobevent_t lrmsevent("LRMSEND", Arc::Time(value));
                    jobevents.push_back(lrmsevent);
                } else if (key == "systemsoftware" ) {
                    extrainfo.insert(
                        std::pair <std::string, std::string>("systemsoftware", value)
                    );
                } else if (key == "wninstance" ) {
                    extrainfo.insert(
                        std::pair <std::string, std::string>("wninstance", value)
                    );
                } else if (key == "benchmark" ) {
                    extrainfo.insert(
                        std::pair <std::string, std::string>("benchmark", value)
                    );
                }
            }
        }
        // Memory: use max if available, otherwise use avarage
        usedmemory = mem_max_resident ? mem_max_resident : mem_avg_resident;
        usedvirtmemory = mem_max_total ? mem_max_total: mem_avg_total;
        // Nodes/CPUs
        if (!nodenames.empty()) {
            // if not recorded in .diag implicitly use nodenames list to count CPUs
            if (!cpucount) {
                cpucount = nodenames.size();
            }
            // add extra info about used nodes
            extrainfo.insert(
                std::pair <std::string, std::string>("nodenames", Arc::join(nodenames, ":"))
            );
            // count the unique nodes
            nodenames.sort();
            nodenames.unique();
            nodecount = nodenames.size();
        }
        // localuser
        std::string localuser = get_file_owner(fname_src);
        if (!localuser.empty()) {
            extrainfo.insert(
                std::pair <std::string, std::string>("localuser", localuser)
            );
        }
              
        /* 
         * analyze job.ID.statistics and store relevant DTR information 
         */
        fname_src = config.ControlDir() + G_DIR_SEPARATOR_S + "job." + job.get_id() + sfx_statistics;
        std::list<std::string> statistics_data;
        // DTR events
        Arc::Time dtr_in_start(-1);
        Arc::Time dtr_in_end(-1);
        Arc::Time dtr_out_start(-1);
        Arc::Time dtr_out_end(-1);
        if (Arc::FileRead(fname_src, statistics_data)) {
            for (std::list<std::string>::iterator line = statistics_data.begin(); line != statistics_data.end(); ++line) {
                // statistics file has : as first delimiter
                std::string::size_type p = line->find(':');
                if (p == std::string::npos) continue;
                std::string key(Arc::lower(line->substr(0, p)));
                std::string value(line->substr(p+1));
                if (key.empty()) continue;
                // new dtr record
                struct aar_data_transfer_t dtrinfo;
                bool is_input = true;
                // key define type of transfer
                if (key == "inputfile") { 
                    dtrinfo.type = dtr_input;
                } else if (key == "outputfile") { 
                    dtrinfo.type = dtr_output;
                    is_input = false;
                }
                // parse comma separated values
                std::list<std::string> dtr_values;
                Arc::tokenize(value, dtr_values, ",");
                std::list<std::string>::iterator it = dtr_values.begin();
                for (std::list<std::string>::iterator it = dtr_values.begin(); it != dtr_values.end(); ++it) {
                    std::string::size_type kvp = it->find('=');
                    if (kvp == std::string::npos) continue;
                    std::string dkey(Arc::lower(it->substr(0, kvp)));
                    std::string dval(it->substr(kvp+1));
                    if (dkey.empty()) continue;
                    if (dkey == "url") {
                        dtrinfo.url = dval;
                    } else if (dkey == "size") {
                        unsigned long long int n;
                        if (string_to_number(dval,n)) dtrinfo.size = n;
                    } else if (dkey == "starttime") {
                        Arc::Time stime(value);
                        dtrinfo.transferstart = stime;
                        if (is_input) {
                            if (dtr_in_start == Arc::Time(-1)) dtr_in_start = stime;
                            if (stime < dtr_in_start) dtr_in_start = stime;
                        } else {
                            if (dtr_out_start == Arc::Time(-1)) dtr_out_start = stime;
                            if (stime < dtr_out_start) dtr_out_start = stime;
                        }
                    } else if (dkey == "endtime") {
                        Arc::Time etime(value);
                        dtrinfo.transferend = etime;
                        if (is_input) {
                            if (dtr_in_end == Arc::Time(-1)) dtr_in_end = etime;
                            if (etime > dtr_in_end) dtr_in_end = etime;
                        } else {
                            if (dtr_out_end == Arc::Time(-1)) dtr_out_end = etime;
                            if (etime > dtr_out_end) dtr_out_end = etime;
                        }
                    } else if (dkey == "fromcache") {
                        if (dval == "yes") {
                            dtrinfo.type = dtr_cache_input;
                        }
                    }
                }
                // total counters: stageinvolume, stageoutvolume
                if ( dtrinfo.type == dtr_input ) stageinvolume += dtrinfo.size;
                if ( dtrinfo.type == dtr_output ) stageoutvolume += dtrinfo.size;
            }
        }
        // Events for data stagein/out
        if (dtr_in_start != Arc::Time(-1) && dtr_in_end != Arc::Time(-1)) {
            aar_jobevent_t dtrstart("DTRDOWNLOADSTART", dtr_in_start);
            jobevents.push_back(dtrstart);
            aar_jobevent_t dtrend("DTRDOWNLOADEND", dtr_in_end);
            jobevents.push_back(dtrend);
        }
        if (dtr_out_start != Arc::Time(-1) && dtr_out_end != Arc::Time(-1)) {
            aar_jobevent_t dtrstart("DTRUPLOADSTART", dtr_out_start);
            jobevents.push_back(dtrstart);
            aar_jobevent_t dtrend("DTRUPLOADEND", dtr_out_end);
            jobevents.push_back(dtrend);
        }
        return true;
    }
}
