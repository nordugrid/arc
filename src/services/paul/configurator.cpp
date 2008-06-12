#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <fstream>
#include <vector>
#include <glibmm.h>
#include <arc/StringConv.h>
#include <arc/ArcRegex.h>
#include <arc/message/PayloadRaw.h>

#include "configurator.h"
#include "job.h"

namespace Paul
{

Configurator::Configurator(Arc::Config *cfg):logger_(Arc::Logger::rootLogger, "Paul::Configurator"),cfg_(*cfg) { 
    cfg_.setFileName(cfg->getFileName());
};

int
Configurator::getPeriod(void)
{
    return Arc::stringtoi((std::string)cfg_["RequestPeriod"]);
}

Arc::XMLNode 
Configurator::getApplicationEnvironments(void)
{
    return cfg_["glue2:ApplicationEnvironments"];
}

std::map<std::string, std::string> 
Configurator::getPki(void)
{
    std::map<std::string, std::string> out;
    out["CertificatePath"] = (std::string)cfg_["CertificatePath"];
    out["PrivateKey"] = (std::string)cfg_["PrivateKey"];
    out["CACertificatePath"] = (std::string)cfg_["CACertificatePath"];
    return out;
}

std::vector<std::string> 
Configurator::getSchedulers(void)
{
    std::vector<std::string> out;
    Arc::XMLNode sched_ep;
    for (int i = 0; (sched_ep = cfg_["SchedulerEndpoint"][i]) != false; i++) {
        std::string sched_endpoint = (std::string)sched_ep;
        out.push_back(sched_endpoint);
    }
    return out;
}

std::string 
Configurator::getCachePath(void)
{
    return (std::string)cfg_["CacheDirectoryPath"];
}

std::string
Configurator::getJobRoot(void)
{
    return (std::string)cfg_["JobRoot"];
}

HTMLRequest::HTMLRequest(Arc::Message &in)
{
    method = in.Attributes()->get("HTTP:METHOD");
    path = in.Attributes()->get("PLEXER:EXTENSION");
    
    // collect POST values
    if (method == "POST") {
        Arc::PayloadRawInterface *in_payload = NULL;
        try {
            in_payload = dynamic_cast<Arc::PayloadRawInterface *>(in.Payload());
        } catch (std::exception &e) {
            throw InvalidMessageException();
        }
        // parse post values
        std::string post_content = in_payload->Content();
        // here assume the post value is in  one line 
        std::vector<std::string> lines;
        Arc::tokenize(post_content, lines, "&");
        for (int i = 0; i < lines.size(); i++) {
            std::vector<std::string> key_value;
            Arc::tokenize(lines[i], key_value, "=");
            if (key_value.size() >= 1) {
                std::string v = Arc::uri_unescape(Glib::strcompress(key_value[1]));
                POST[key_value[0]] = v; 
            }
        }
    }
    // set base_path
    std::string endpoint = in.Attributes()->get("HTTP:ENDPOINT");
    base_path = endpoint.substr(0, endpoint.rfind(path));
    if (base_path[base_path.size()] != '/') {
        base_path += '/';
    }
}

void
Configurator::style(Configurator *self, HTMLRequest &request, HTMLResponse &response)
{
    response += Glib::file_get_contents("style.css");
    response.content_type = "text/plain";
}

void 
Configurator::index(Configurator *self, HTMLRequest &request, HTMLResponse &response)
{
    response += response.header;
    response += "<ul>";
    response += "<li><a href=\"" + request.base_path + "jobs/\">Jobs</a></li>";
    response += "<li><a href=\"" + request.base_path + "conf/\">Basic Coniguration Options</a></li>";
    response += "<li><a href=\"" + request.base_path + "sched/\">Schedulers</a></li>";
    response += "<li><a href=\"" + request.base_path + "rte/\">Application Environments</a></li>";
    response += "<li><a href=\"" + request.base_path + "log/\">Log messages</a></li>";
    response += "</ul>";
    response += response.footer;
}

void 
Configurator::conf(Configurator *self, HTMLRequest &request, HTMLResponse &response)
{
    response += response.header;
    response += "<div id=\"content-table\">";
    response += "<table border=\"0\" cellpading=\"5px\" cellspacing=\"5px\">";
    response += "<tr><td>Period (s)</td><td>" + Arc::tostring(self->getPeriod()) + "</td></tr>";
    response += "<tr><td>Job root directory</td><td>" + self->getJobRoot() + "</td></tr>";
    response += "<tr><td>Client certificate path</td><td>" + self->getPki()["CertificatePath"] + "</td></tr>";
    response += "<tr><td>Client private key path</td><td>" + self->getPki()["PrivateKey"] + "</td></tr>";
    response += "<tr><td>Accepted CAs path</td><td>" + self->getPki()["CACertificatePath"] + "</td></tr>";
    response += "</table></div>";
    response += response.footer;
}

void 
Configurator::sched(Configurator *self, HTMLRequest &request, HTMLResponse &response)
{
    response += response.header;
    response += "<div id=\"content-div\"> \
                    <div id=\"content-title\"> \
                        <div id=\"button\"><a href=\"" + request.base_path + "sched/add/\">add</a></div> \
                        <div id=\"title-name\"><h2>Current Schedulers</h2></div> \
                    </div>";
    response += "<div id=\"content-content\"><table border=\"0\" cellpadding=\"5px\" cellspacing=\"5px\" width=\"100%\">";
    response += "<tr><th>URL</th><th>Actions</th></tr>";
    std::vector<std::string> schedulers = self->getSchedulers();
    for (int i = 0; i < schedulers.size(); i++) {
        response += "<tr><td width=\"100%\">" + schedulers[i] + "</td><td><a href=\"" + request.base_path + "sched/del/" + Arc::tostring(i) + "/\">delete</a></td></tr>";
    }
    response += "</table></div></div>";
    response += response.footer;
}

void 
Configurator::sched_add(Configurator *self, HTMLRequest &request, HTMLResponse &response)
{
    response += response.header;
    if (request.method == "GET") {
        // show form
        response += "<div id=\"content-div\"> \
                    <div id=\"content-title\"> \
                        <div id=\"title-name\"><h2>Add Scheduler</h2></div> \
                    </div>";
        response += "<div id=\"content-content\">";
        response += "<form action=\".\" method=\"post\">";
        response += "<p><label>URL:</label><input type=\"text\" name=\"sched_url\" id=\"sched_url\"/></p>";
        response += "<p><input type=\"submit\" value=\"ADD\"/></p>";
        response += "</form>";
        response += "</div></div>";
    } else if (request.method == "POST") {
        // process form
        std::string sched_url = request.POST["sched_url"];
        if (!sched_url.empty()) {
            Arc::Config cfg;
            cfg.parse(self->cfg_.getFileName().c_str());
            // find service tag
            Arc::XMLNode chain = cfg["Chain"];
            Arc::XMLNode service;
            for (int i = 0; (service = chain["Service"][i]) != false; i++) {
                if ("paul" == (std::string)service.Attribute("name")) {
                    break;
                }
            }
            service.NewChild("paul:SchedulerEndpoint") = sched_url;
            cfg.save(self->cfg_.getFileName().c_str());
            Arc::Config new_cfg(service, self->cfg_.getFileName());
            new_cfg.New(self->cfg_);
            response += "<p id=\"msg\">Scheduler url: <b>" + sched_url + "</b> has been added.</p>";
            response += "<p><a href=\"" + request.base_path + "sched/\">&#171; Back</a></p>";
        }
    }
    response += response.footer;
}

void 
Configurator::sched_del(Configurator *self, HTMLRequest &request, HTMLResponse &response)
{
    std::vector<std::string> tokens;
    Arc::tokenize(request.path, tokens, "/");
    self->logger_.msg(Arc::DEBUG, "** %s", request.path);
    response += response.header;
    if (tokens.size() == 3) {
        int sched_id = Arc::stringtoi(tokens[2]);
        Arc::Config cfg;
        cfg.parse(self->cfg_.getFileName().c_str());
        // find service tag
        Arc::XMLNode chain = cfg["Chain"];
        Arc::XMLNode service;
        for (int i = 0; (service = chain["Service"][i]) != false; i++) {
            if ("paul" == (std::string)service.Attribute("name")) {
                break;
            }
        }
        Arc::XMLNode sched;
        std::vector<std::string> schedulers = self->getSchedulers();
        for (int i = 0; (sched = service["SchedulerEndpoint"][i]) != false; i++) {
            if (schedulers[sched_id] == (std::string)sched) {
               sched.Destroy();
            }
        }
        cfg.save(self->cfg_.getFileName().c_str());
        Arc::Config new_cfg(service, self->cfg_.getFileName());
        response += "<p id=\"msg\"><b>" + self->getSchedulers()[sched_id] + "</b> has been removed.</p>";
        new_cfg.New(self->cfg_);
    } else {
        response += "<p style=\"err_msg\">No such scheduler!</p>";
    }
    response += "<p><a href=\"" + request.base_path + "sched/\">&#171; Back</a></p>";
    response += response.footer;
}

std::string 
tail(std::string file_name, int line_n)
{
    std::ifstream fin(file_name.c_str());
    int ln = 0;
    int seek_size = 1024;
    std::list<std::string> lines;
    fin.seekg(0, std::ios::end);
    int block_end = fin.tellg(); 
    fin.seekg(-seek_size, std::ios::end);
    int pos = fin.tellg();
    int block_start = pos;
/*
    std::cout << "bs: " << block_start << std::endl;
    std::cout << "p: " << pos << std::endl;
    std::cout << "be: " << block_end << std::endl;
*/
    std::vector<std::string> tokens;
    char buffer[1024];
    for(;;) { 
        memset(buffer, 0, sizeof(buffer));
        fin.read(buffer, sizeof(buffer));
        buffer[sizeof(buffer) - 1] = '\0';
        std::string l = buffer;
        tokens.clear();
        Arc::tokenize(l, tokens, "\n");
        for (int i = tokens.size()-1 ; i >= 0; i--) {
// std::cout << "**" << tokens[i] << "(" << i << "/" << tokens.size() << ")" << std::endl;
            if (i != 0) {
                lines.push_back(tokens[i] + "\n");
                ln++;
            } else {
                lines.push_back(tokens[i]);
            }
        }
        if (ln >= line_n) {
            break;
        }
        pos = fin.tellg();
// std::cout << pos << std::endl;
        if ((fin.eof() && ln < line_n) || pos >= block_end) {
            if (block_start == 0) {
                break;
            }
            block_end = block_start;
            int s = block_start - seek_size;
            if (s < 0) {
                s = 0;
            }
            fin.seekg(s, std::ios::beg);
            block_start = fin.tellg();
        }
/*
    std::cout << "bs: " << block_start << std::endl;
    std::cout << "p: " << pos << std::endl;
    std::cout << "be: " << block_end << std::endl;
*/
    }
    fin.close();
    std::list<std::string>::iterator it;
    std::string out;
    std::string prev;
    for (it = lines.begin(); it != lines.end(); it++) {
        std::string l = *it;
        if (l[l.size() - 1] != '\n') {
            prev = l;
        } else {
            if (!prev.empty()) {
                l += prev;
                prev.clear();
            }
            out += ("<p class=\"log\">" + l + "</p>");
        }
    }
    return out;
}

void
Configurator::log(Configurator *self, HTMLRequest &request, HTMLResponse &response)
{
    response += response.header;
    Arc::Config cfg;
    cfg.parse(self->cfg_.getFileName().c_str());
    std::string filename = (std::string)cfg["Server"]["Logger"];
    if (!filename.empty()) {
        response += tail(filename, 100); 
    }
    response += response.footer;
}

void
Configurator::rte(Configurator *self, HTMLRequest &request, HTMLResponse &response)
{
    response += response.header;
    response += "<div id=\"content-div\"> \
                    <div id=\"content-title\"> \
                        <div id=\"button\"><a href=\"" + request.base_path + "rte/add/\">add</a></div> \
                        <div id=\"title-name\"><h2>Current Application Environments</h2></div> \
                    </div>";
    response += "<div id=\"content-content\"><table border=\"0\" cellpadding=\"5px\" cellspacing=\"5px\" width=\"100%\">";
    response += "<tr><th>Name</th><th>Version</th><th>Actions</th></tr>";
    Arc::XMLNode rtes = self->getApplicationEnvironments();
    Arc::XMLNode rte;
    for (int i = 0; (rte = rtes["ApplicationEnvironment"][i]) != false; i++) {
        response += "<tr><td>" + (std::string)rte["Name"] + "</td><td>" + (std::string)rte["Version"] + "</td><td><a href=\"" + request.base_path + "rte/del/" + Arc::tostring(i) + "/\">delete</a></td></tr>";
    }
    response += "</table></div></div>";
    response += response.footer;
}

void
Configurator::rte_add(Configurator *self, HTMLRequest &request, HTMLResponse &response)
{
    response += response.header;
    if (request.method == "GET") {
        // show form
        response += "<div id=\"content-div\"> \
                    <div id=\"content-title\"> \
                        <div id=\"title-name\"><h2>Add Application Environment</h2></div> \
                    </div>";
        response += "<div id=\"content-content\">";
        response += "<form action=\".\" method=\"post\">";
        response += "<p><label>Name: </label><input type=\"text\" name=\"rte_name\" id=\"rte_name\"/></p>";
        response += "<p><label>Version: </label><input type=\"text\" name=\"rte_version\" id=\"rte_version\"/></p>";
        response += "<p><input type=\"submit\" value=\"ADD\"/></p>";
        response += "</form>";
        response += "</div></div>";
    } else if (request.method == "POST") {
        // process form
        std::string rte_name = request.POST["rte_name"];
        std::string rte_version = request.POST["rte_version"];
        if (!rte_name.empty()) {
            Arc::Config cfg;
            cfg.parse(self->cfg_.getFileName().c_str());
            // find service tag
            Arc::XMLNode chain = cfg["Chain"];
            Arc::XMLNode service;
            for (int i = 0; (service = chain["Service"][i]) != false; i++) {
                if ("paul" == (std::string)service.Attribute("name")) {
                    break;
                }
            }
            Arc::XMLNode rtes = service["ApplicationEnvironments"];
            // XXX get glue2 namespace
            Arc::XMLNode r = rtes.NewChild("glue2:ApplicationEnvironment");
            r.NewChild("glue2:Name") = rte_name;
            r.NewChild("glue2:Version") = rte_version;
            cfg.save(self->cfg_.getFileName().c_str());
            Arc::Config new_cfg(service, self->cfg_.getFileName());
            new_cfg.New(self->cfg_);
            response += "<p id=\"msg\">Application Environment: <b>" + rte_name + "</b> has been added.</p>";
            response += "<p><a href=\"" + request.base_path + "rte/\">&#171; Back</a></p>";
        }
    }
    response += response.footer;
}

void
Configurator::rte_del(Configurator *self, HTMLRequest &request, HTMLResponse &response)
{
    std::vector<std::string> tokens;
    Arc::tokenize(request.path, tokens, "/");
    response += response.header;
    if (tokens.size() == 3) {
        int rte_id = Arc::stringtoi(tokens[2]);
        Arc::Config cfg;
        cfg.parse(self->cfg_.getFileName().c_str());
        // find service tag
        Arc::XMLNode chain = cfg["Chain"];
        Arc::XMLNode service;
        for (int i = 0; (service = chain["Service"][i]) != false; i++) {
            if ("paul" == (std::string)service.Attribute("name")) {
                break;
            }
        }
        Arc::XMLNode rtes = service["ApplicationEnvironments"];
        Arc::XMLNode rte = rtes["ApplicationEnvironment"][rte_id];
        if (rte != false) {
            rte.Destroy();
        }
        cfg.save(self->cfg_.getFileName().c_str());
        Arc::Config new_cfg(service, self->cfg_.getFileName());
        response += "<p id=\"msg\">Application Environment has been removed.</p>";
        new_cfg.New(self->cfg_);
    } else {
        response += "<p style=\"err_msg\">No such Application Environment!</p>";
    }
    response += "<p><a href=\"" + request.base_path + "rte/\">&#171; Back</a></p>";
    response += response.footer;
}

void 
Configurator::jobs(Configurator *self, HTMLRequest &request, HTMLResponse &response)
{
    response += response.header;
    response += "<div id=\"content-div\"> \
                    <div id=\"content-title\"> \
                        <div id=\"title-name\"><h2>Current Jobs</h2></div> \
                    </div>";
    response += "<div id=\"content-content\"><table border=\"0\" cellpadding=\"5px\" cellspacing=\"5px\" width=\"100%\">";
    response += "<tr><th>UUID</th><th>Name</th><th>Status</th><th>Scheduler</th><th>Actions</th></tr>";
    std::map<const std::string, Job *> all = self->getJobQueue()->getAllJobs();
    std::map<const std::string, Job *>::iterator it;
    for (it = all.begin(); it != all.end(); it++) {
        Job *j = it->second;
        std::string s = sched_status_to_string(j->getStatus());
        std::string id = j->getID();
        std::string name = j->getJobRequest().getName();
        std::string sched_url = j->getResourceID();
        response += "<tr><td>" + id + "</td><td>" + name + "</td><td>" + s + "</td><td>" + sched_url + "</td><td> \
                    <a href=\"" + request.base_path + "job/" + id + "/\">detail</a> | \
                    <a href=\"" + request.base_path + "job/stop/" + id + "/\">stop</a></td></tr>";
    }
    response += "</table></div></div>";
    response += response.footer;
}

void 
Configurator::job(Configurator *self, HTMLRequest &request, HTMLResponse &response)
{
}

void 
Configurator::job_stop(Configurator *self, HTMLRequest &request, HTMLResponse &response)
{
}

Arc::MCC_Status
Configurator::process(Arc::Message &in, Arc::Message &out)
{

    Controller ctrl;
    ctrl.insert(std::make_pair("^$", &index));
    ctrl.insert(std::make_pair("^/$", &index));
    ctrl.insert(std::make_pair("^/style/$", &style));
    ctrl.insert(std::make_pair("^/conf/$", &conf));
    ctrl.insert(std::make_pair("^/sched/$", &sched));
    ctrl.insert(std::make_pair("^/sched/add/$", &sched_add));
    ctrl.insert(std::make_pair("^/sched/del/[0-9]*/$", &sched_del));
    ctrl.insert(std::make_pair("^/log/$", &log));
    ctrl.insert(std::make_pair("^/rte/$", &rte));
    ctrl.insert(std::make_pair("^/rte/add/$", &rte_add));
    ctrl.insert(std::make_pair("^/rte/del/[0-9]*/$", &rte_del));
    ctrl.insert(std::make_pair("^/jobs/$", &jobs));
    ctrl.insert(std::make_pair("^/job/[a-f0-9\\\\-]*/$", &job));
    ctrl.insert(std::make_pair("^/job/stop/[a-f0-9\\\\-]*/$", &job_stop));

    HTMLRequest request(in);
    Arc::PayloadRaw *out_buf = new Arc::PayloadRaw;
    if (!out_buf) {
        logger_.msg(Arc::ERROR, "Cannot callocate output raw buffer");
        return Arc::MCC_Status();
    }
    Controller::iterator it;
    for (it = ctrl.begin(); it != ctrl.end(); it++) {
        Arc::RegularExpression r(it->first);
        if (r.match(request.path) == true) {
            HTMLResponse response;
            // set html header
            response.header = "<html><header><title>ARC - Paul Service Configurator</title></header><link rel=\"stylesheet\" type=\"text/css\" href=\"" + request.base_path + "style/\"/><body><div id=\"header\"><h1><a href=\"" + request.base_path + "\"/>ARC - Paul Service Configurator</a></h1></div><div id=\"content\">";
            // set footer
            response.footer = "</div></body></html>";
            // call viewer function
            (*(it->second))(this, request, response);
            // generate output message
            out_buf->Insert(response.html.c_str(), 0, response.html.length());
            out.Payload(out_buf);
            out.Attributes()->set("HTTP:content-type", response.content_type);
            return Arc::MCC_Status(Arc::STATUS_OK);
        }
    }
    std::string html_error = "<body><html><h1>Page not found</h1></html></body>";
    out_buf->Insert(html_error.c_str(), 0, html_error.length());
    out.Payload(out_buf);
    out.Attributes()->set("HTTP:content-type", "text/html");
    // how to set HTTP 404
    return Arc::MCC_Status(Arc::STATUS_OK);

#if 0
    if (client_host != "127.0.0.1") {
        logger_.msg(Arc::ERROR, "Permission denied from %s host", client_host);
        return Arc::MCC_Status();
    }
#endif
}

} // namespace Paul
