#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <glibmm/fileutils.h>
#include <glibmm/miscutils.h>
#include <arc/loader/Loader.h>
#include <arc/loader/ServiceLoader.h>

#include "httpd.h"
#include "PayloadFile.h"

namespace HTTPD {

static Arc::Service *get_service(Arc::Config *cfg, Arc::ChainContext*)
{
    return new HTTPD(cfg);
}

HTTPD::HTTPD(Arc::Config *cfg):Service(cfg), logger(Arc::Logger::rootLogger, "HTTPD")
{
    logger.msg(Arc::INFO, "HTTPD Initialized"); 
    doc_root = (std::string)((*cfg)["DocumentRoot"]);
    if (doc_root.empty() == 0) {
        doc_root = "./";
    }
}

HTTPD::~HTTPD(void)
{
    logger.msg(Arc::INFO, "HTTPD shutdown");
}

Arc::PayloadRawInterface *HTTPD::Get(const std::string &path, const std::string &base_url)
{
    // XXX eliminate relativ paths first
    std::string full_path = Glib::build_filename(doc_root, path);
    if (Glib::file_test(full_path, Glib::FILE_TEST_EXISTS) == true) {
        if (Glib::file_test(full_path, Glib::FILE_TEST_IS_REGULAR) == true) {
            return new PayloadFile(full_path.c_str());
        } else if (Glib::file_test(full_path, Glib::FILE_TEST_IS_DIR)) {
            std::string html = "<HTML>\r\n<HEAD>Directory list of '" + path + "'</HEAD>\r\n<BODY><UL>\r\n";
            Glib::Dir dir(full_path);
            std::string d;
            while ((d = dir.read_name()) != "") {
                html += "<LI><a href=\"/"+base_url+"/"+d+"\">"+d+"</a></LI>\r\n";
            }
            html += "</UL></BODY></HTML>";
            Arc::PayloadRaw *buf = new Arc::PayloadRaw();
            buf->Insert(html.c_str(), 0, html.length());
            return buf;
        }
    }
    return NULL;
}

Arc::MCC_Status HTTPD::Put(const std::string &path, Arc::PayloadRawInterface &buf)
{
    // XXX eliminate relativ paths first
    logger.msg(Arc::DEBUG, "PUT called");
    std::string full_path = Glib::build_filename(doc_root, path);
    int fd = open(full_path.c_str(), O_CREAT|O_WRONLY|O_TRUNC, 0600);
    if (fd == -1) {
        logger.msg(Arc::ERROR, strerror(errno));        
        return Arc::MCC_Status();
    }
    for(int n = 0;;++n) {
        char* sbuf = buf.Buffer(n);
        if(sbuf == NULL) break;
        off_t offset = buf.BufferPos(n);
        size_t size = buf.BufferSize(n);
        if(size > 0) {
            off_t o = lseek(fd, offset, SEEK_SET);
            if(o != offset) {
                close(fd);
                unlink(full_path.c_str());
                logger.msg(Arc::DEBUG, "error on seek");
                return Arc::MCC_Status();
            }
            for(;size>0;) {
                ssize_t l = write(fd, sbuf, size);
                if(l == -1) {
                    close(fd);
                    unlink(full_path.c_str());
                    logger.msg(Arc::DEBUG, "error on write");
                    return Arc::MCC_Status();
                }
                size-=l; sbuf+=l;
            }
        }
    }
    close(fd);
    return Arc::MCC_Status(Arc::STATUS_OK);
}

Arc::MCC_Status HTTPD::process(Arc::Message &inmsg, Arc::Message &outmsg)
{
    std::string method = inmsg.Attributes()->get("HTTP:METHOD");
    std::string url = inmsg.Attributes()->get("HTTP:ENDPOINT");
    std::string path;
    
    // cut id from the begining of url
    while(url[0] == '/') url=url.substr(1);
    std::string::size_type p = url.find('/');
    if(p != std::string::npos) {
       path = url.substr(p);
       while(path[0] == '/') path=path.substr(1);
    }

    logger.msg(Arc::INFO, "method=%s, path=%s", method.c_str(), path.c_str());
    if (method == "GET") {
        Arc::PayloadRawInterface *buf = Get(path, url);
        if (!buf) {
            // XXX: HTTP error
            return Arc::MCC_Status();
        }
        outmsg.Payload(buf);
        return Arc::MCC_Status(Arc::STATUS_OK);
    } else if (method == "PUT") {
        Arc::PayloadRawInterface *inbufpayload = NULL;
        try {
            inbufpayload = dynamic_cast<Arc::PayloadRawInterface *>(inmsg.Payload());
        } catch (std::exception &e) {
            logger.msg(Arc::ERROR, "Error while processing input: %s", e.what());
            return Arc::MCC_Status();
        }
        if (inbufpayload) {
            Arc::MCC_Status ret = Put(path, *inbufpayload);
            if (!ret) {
                // XXX: HTTP error
                return Arc::MCC_Status();
            }
            Arc::PayloadRaw *buf = new Arc::PayloadRaw();
            outmsg.Payload(buf);
            return ret;
        }
    } 
    logger.msg(Arc::WARNING, "Not supported operation");
    return Arc::MCC_Status();
}

} // namesapce HTTPD

service_descriptors ARC_SERVICE_LOADER = {
    { "httpd", 0, &HTTPD::get_service },
    { NULL, 0, NULL}
};
