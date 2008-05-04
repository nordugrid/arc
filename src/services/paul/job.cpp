#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glibmm.h>
#include "job.h"
#include <arc/GUID.h>
#include <arc/StringConv.h>
#include <iostream>
#include <fstream>
#include <unistd.h>

namespace Paul {

Job::Job(void) 
{
    finished_reported = false;
    timeout = 5;
    check = 0;
}

Job::Job(const Job &j)
{
    request = j.request;
    sched_meta = j.sched_meta;
    timeout = j.timeout;
    id = j.id;
    db = j.db;
    check = j.check;
    finished_reported = false;
}

Job::Job(JobRequest &r)
{
    request = r;
    finished_reported = false;
}

Job::Job(JobRequest &r, JobSchedMetaData &m, int t, const std::string &db_path) 
{
    request = r;
    sched_meta = m;
    timeout = t;
    id = Arc::UUID();
    db = db_path;
    check = 0;
    finished_reported = false;
}

Job::Job(const std::string &jobid, const std::string &db_path) 
{
    id = jobid;
    db = db_path;
    check = 0;
    timeout = 5;
    finished_reported = false;
}

Job::Job(std::istream &job, const std::string &db_path) 
{
    db = db_path;
    std::string xml_document;
    std::string xml_line;
    Arc::XMLNode tmp_xml;
    check = 0;
    timeout = 5;

    while (getline(job, xml_line)) {
        xml_document += xml_line;
    }
    (Arc::XMLNode (xml_document)).New(tmp_xml);
    JobRequest job_desc(tmp_xml);
    setJobRequest(job_desc);
}

Job::~Job(void) 
{
    // NOP
}

bool Job::CheckTimeout(void) 
{
    check++;
    if (check < timeout) {
        return true;
    }
    
    check = 0;
    return false;
}

bool Job::Cancel(void) 
{
    status = KILLED;
    return true;
}

inline void write_pair(std::ofstream &f, std::string name,std::string &value) 
{
    f << name << '=' << value << std::endl;
}

bool Job::save(void) 
{ 
    // write out the jobrequest
    Arc::XMLNode jsdl = getJSDL();
    std::string jsdl_str;
    std::string fname = db + "/" + id + ".jsdl";
    jsdl.GetXML(jsdl_str);
    std::ofstream f1(fname.c_str(), std::ios::out | std::ios::trunc);
    if(!f1.is_open()) {
        return false; /* can't open file */
    }
    f1 << jsdl_str;
    f1.close();
    // write out job metadata
    fname = db + "/" + id + ".metadata";
    std::ofstream f2(fname.c_str(), std::ios::out | std::ios::trunc);
    if(!f2.is_open()) {
        return false; /* can't open file */
    }
    write_pair(f2, "id", id);
    std::string arex_id = getResourceID();
    write_pair(f2, "arex_id", arex_id);
    std::string status_str = sched_status_to_string(status);
    write_pair(f2, "status", status_str);
    f2.close();
    
    // write out arex_job_id
    fname = db + "/" + id + ".arex_job_id";
    jsdl.GetXML(jsdl_str);
    std::ofstream f3(fname.c_str(), std::ios::out | std::ios::trunc);
    if(!f3.is_open()) {
        return false; /* can't open file */
    }
    f3 << sched_meta.getResourceID();
    f3.close();

    return true;
}

bool cut(std::string &input, std::string &name, std::string &value) 
{
    int size = input.size();
    int i = input.find_first_of("=");
    if (i == std::string::npos) {
        return false;
    }
    name = input.substr(0, i);
    value = input.substr(i+1, size);
    return true;
}

bool Job::load(void) 
{
    char buf[250];
    std::string fname = db + "/" + id + ".metadata";
    std::ifstream f(fname.c_str());
    if (!f.is_open()) {
        return false;
    }
    for (;!f.eof();) {
        f.getline(buf, 250);
        std::string line(buf);
        std::string name;
        std::string value;
        if (!cut(line,name,value)) {
            continue;
        }
        if (name == "id") {
            id = value;
        } else if (name == "arex_id") {
            sched_meta.setResourceID(value);
        } else if (name == "status") {
            status = sched_status_from_string(value);
        }
    }
    f.close(); 

    // read jsdl
    std::string fname_jsdl = db + "/" + id + ".jsdl";
    std::ifstream f_jsdl(fname_jsdl.c_str());
    std::string xml_document;
    std::string xml_line;
    Arc::XMLNode tmp_xml;
    while (getline(f_jsdl, xml_line)) {
        xml_document += xml_line;
    }
    (Arc::XMLNode (xml_document)).New(tmp_xml);
    f_jsdl.close();
    JobRequest job_desc(tmp_xml);
    setJobRequest(job_desc);
    //read arex_job_id
    std::string a_id = db + "/" + id + ".arex_job_id";
    std::ifstream f_arex(a_id.c_str());
    std::string line, tmp;
    while (getline(f_arex, line)) {
        tmp += line;
    }
    sched_meta.setResourceID(tmp);

    f_arex.close();
    return true;
}

bool Job::remove(void) 
{
    std::string file1 = db + "/" + id + ".metadata";
    std::string file2 = db + "/" + id + ".jsdl";
    std::string file3 = db + "/" + id + ".arex_job_id";
    std::remove(file1.c_str());
    std::remove(file2.c_str());
    std::remove(file3.c_str());

    return true;
}

static void Remove(const std::string &path)
{
    if (Glib::file_test(path, Glib::FILE_TEST_IS_REGULAR)) {
        unlink(path.c_str());
        return;
    }
    if (Glib::file_test(path, Glib::FILE_TEST_IS_DIR)) {
        Glib::Dir dir(path);
        std::string d;
        while ((d = dir.read_name()) != "") {
            Remove(Glib::build_filename(path, d));
        }
        dir.close();
        rmdir(path.c_str());
    }
}

void Job::clean(const std::string &jobroot)
{
    std::string wd = Glib::build_filename(jobroot, id);
    Remove(wd);
}

const std::string Job::getFailure(void)
{
    return failure + "/" + sched_meta.getFailure();
}
}
