#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <glibmm/fileutils.h>
#include <glibmm/miscutils.h>
#include <arc/message/MessageAttributes.h>
#include <arc/message/PayloadRaw.h>
#include <arc/message/PayloadStream.h>
#include <arc/Utils.h>


#include "hopi.h"
#include "PayloadFile.h"

namespace Hopi {

static Arc::Plugin *get_service(Arc::PluginArgument* arg)
{
    Arc::MCCPluginArgument* mccarg =
            arg?dynamic_cast<Arc::MCCPluginArgument*>(arg):NULL;
    if(!mccarg) return NULL;
    return new Hopi((Arc::Config*)(*mccarg));
}

Hopi::Hopi(Arc::Config *cfg):Service(cfg), logger(Arc::Logger::rootLogger, "Hopi")
{
    logger.msg(Arc::INFO, "Hopi Initialized"); 
    doc_root = (std::string)((*cfg)["DocumentRoot"]);
    if (doc_root.empty() == 1) {
        doc_root = "./";
    }
    logger.msg(Arc::INFO, "Hopi DocumentRoot is " + doc_root);
    slave_mode = (std::string)((*cfg)["SlaveMode"]);
    if (slave_mode.empty() == 1) {
        slave_mode = "0";
    }  
    if (slave_mode == "1") logger.msg(Arc::INFO, "Hopi SlaveMode is on!");
}

Hopi::~Hopi(void)
{
    logger.msg(Arc::INFO, "Hopi shutdown");
}

Arc::PayloadRawInterface *Hopi::Get(const std::string &path, const std::string &base_url)
{
    // XXX eliminate relativ paths first
    std::string full_path = Glib::build_filename(doc_root, path);
    if (Glib::file_test(full_path, Glib::FILE_TEST_EXISTS) == true) {
        if (Glib::file_test(full_path, Glib::FILE_TEST_IS_REGULAR) == true) {
            PayloadFile * pf = new PayloadFile(full_path.c_str());
            if (slave_mode == "1") unlink(full_path.c_str());
            return pf;
        } else if (Glib::file_test(full_path, Glib::FILE_TEST_IS_DIR) && slave_mode != "1") {
            std::string html = "<HTML>\r\n<HEAD>Directory list of '" + path + "'</HEAD>\r\n<BODY><UL>\r\n";
            Glib::Dir dir(full_path);
            std::string d;
            std::string p;
            if (path == "/") {
                p = "";
            } else {
                p = path;
            }
            while ((d = dir.read_name()) != "") {
                html += "<LI><a href=\""+ base_url + p + "/"+d+"\">"+d+"</a></LI>\r\n";
            }
            html += "</UL></BODY></HTML>";
            Arc::PayloadRaw *buf = new Arc::PayloadRaw();
            buf->Insert(html.c_str(), 0, html.length());
            return buf;
        }
    }
    return NULL;
}

Arc::MCC_Status Hopi::Put(const std::string &path, Arc::MessagePayload &payload)
{
    // XXX eliminate relativ paths first
    logger.msg(Arc::DEBUG, "PUT called");
    std::string full_path = Glib::build_filename(doc_root, path);
    if ((slave_mode == "1") && (Glib::file_test(full_path, Glib::FILE_TEST_EXISTS) == false)) {
        logger.msg(Arc::ERROR, "Hopi SlaveMode is active, PUT is only allowed to existing files");        
        return Arc::MCC_Status();
    }
    int fd = open(full_path.c_str(), O_CREAT|O_WRONLY|O_TRUNC, 0600);
    if (fd == -1) {
        logger.msg(Arc::ERROR, Arc::StrError(errno));        
        return Arc::MCC_Status();
    }
    try {
        Arc::PayloadStreamInterface& stream = dynamic_cast<Arc::PayloadStreamInterface&>(payload);
        const int bufsize = 1024*1024;
        char* sbuf = new char[bufsize];
        for(;;) {
            int size = bufsize;
            if(!stream.Get(sbuf,size)) {
                if(!stream) {
                    delete[] sbuf;
                    close(fd);
                    unlink(full_path.c_str());
                    logger.msg(Arc::DEBUG, "error reading from HTTP stream");
                    return Arc::MCC_Status();
                }
            }
            for(;size>0;) {
                ssize_t l = write(fd, sbuf, size);
                if(l == -1) {
                    delete[] sbuf;
                    close(fd);
                    unlink(full_path.c_str());
                    logger.msg(Arc::DEBUG, "error on write");
                    return Arc::MCC_Status();
                }
                size-=l; sbuf+=l;
            }
        }
        delete[] sbuf;
    } catch (std::exception &e) {
        try {
            Arc::PayloadRawInterface& buf = dynamic_cast<Arc::PayloadRawInterface&>(payload);
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
        } catch (std::exception &e) {
            close(fd);
            logger.msg(Arc::ERROR, "Input for PUT operation is neither stream nor buffer");
            return Arc::MCC_Status();
        }
    }
    close(fd);
    if (slave_mode == "1") unlink(full_path.c_str());
    return Arc::MCC_Status(Arc::STATUS_OK);
}

Arc::MCC_Status Hopi::process(Arc::Message &inmsg, Arc::Message &outmsg)
{
    std::string method = inmsg.Attributes()->get("HTTP:METHOD");
    std::string url = inmsg.Attributes()->get("HTTP:ENDPOINT");
    std::string path;
    std::string base_url;
    Arc::AttributeIterator iterator = inmsg.Attributes()->getAll("PLEXER:EXTENSION");
    if (iterator.hasMore()) {
        path = *iterator;
        base_url = url.substr(0, url.length() - path.length());    
    } else {
        path = url;
        base_url = "";
    }

    logger.msg(Arc::DEBUG, "method=%s, path=%s, url=%s, base_url=%s", method, path, url, base_url);
    if (method == "GET") {
        Arc::PayloadRawInterface *buf = Get(path, base_url);
        if (!buf) {
            // XXX: HTTP error
            return Arc::MCC_Status();
        }
        outmsg.Payload(buf);
        return Arc::MCC_Status(Arc::STATUS_OK);
    } else if (method == "PUT") {
        Arc::MessagePayload *inpayload = inmsg.Payload();
        if(!inpayload) {
            logger.msg(Arc::WARNING, "No content provided for PUT operation");
            return Arc::MCC_Status();
        }
        Arc::MCC_Status ret = Put(path, *inpayload);
        if (!ret) {
            // XXX: HTTP error
            return Arc::MCC_Status();
        }
        Arc::PayloadRaw *buf = new Arc::PayloadRaw();
        outmsg.Payload(buf);
        return ret;
    } 
    logger.msg(Arc::WARNING, "Not supported operation");
    return Arc::MCC_Status();
}

} // namespace Hopi

Arc::PluginDescriptor PLUGINS_TABLE_NAME[] = {
    { "hopi", "HED:SERVICE", 0, &Hopi::get_service },
    { NULL, NULL, 0, NULL}
};
