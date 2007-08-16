#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>

#include <string>

#define MAX_CHUNK_SIZE (10*1024*1024)

namespace ARex {

static bool http_get(const std::string& burl,const std::string& bpath,const std::string& hpath,uint64_t& offset,uint64_t& size,Arc::PayloadRawInterface& buf);

Arc::MCC_Status Get(ARexGMConfig& config,const std::string& id,const std::string& subpath,Arc::PayloadRawInterface& buf) {
  ARexJob job(id,config);
  if(!job) {
    // There is no such job

    return Arc::MCC_Status();
  };
  std::string session_dir = job.SesssionDir();
  uint64_t offset = 0;
  uint64_t size = 0;
  http_get(config.Endpoint()+id,session_dir,subpath,offset,size)

} 

static bool http_get(const std::string& burl,const std::string& bpath,const std::string& hpath,uint64_t& offset,uint64_t& size,Arc::PayloadRawInterface& buf) {
  std::string path=bpath+"/"+hpath;
  struct stat64 st;
  if(lstat64(path.c_str(),&st) == 0) {
    if(S_ISDIR(st.st_mode)) {
      DIR *dir=opendir(path.c_str());
      if(dir != NULL) {
        // Directory - html with file list
        struct dirent file_;
        struct dirent *file;
        std::string html;
        html="<HTML>\r\n<HEAD></HEAD>\r\n<BODY><UL>\r\n";
        for(;;) {
          readdir_r(dir,&file_,&file);
          if(file == NULL) break;
          std::string fpath = path+"/"+file->d_name;
          if(lstat64(fpath.c_str(),&st) == 0) {
            if(S_ISREG(st.st_mode)) {
              std::string line = "<LI><I>file</I> <A HREF=\"";
              line+=burl+"/"+bpath+"/"+file->d_name;
              line+="\">";
              line+=file->d_name;
              line+="</A>\r\n";
              html+=line;
            } else if(S_ISDIR(st.st_mode)) {
              std::string line = "<LI><I>dir</I> <A HREF=\"";
              line+=burl+"/"+bpath+"/"+file->d_name+"/";
              line+="\">";
              line+=file->d_name;
              line+="</A>\r\n";
              html+=line;
            };
          };
        };
        closedir(dir);
        html+="</UL>\r\n</BODY>\r\n</HTML>";
        buf.Insert(html.c_str(),0,html.length());
        return true;
      };
    };
  } else if(S_ISREG(st.st_mode)) {
    // File 
    int h = open64(path.c_str(),O_RDONLY);
    if(h != -1) {
      off_t o = lseek64(h,offset,SEEK_SET);
      if(o != offset) {
        // Out of file

      } else {
        if(size > MAX_CHUNK_SIZE) size=MAX_CHUNK_SIZE;
        char* sbuf = buf.Insert(offset,size);
        if(buf) {
          ssize_t l = read(h,sbuf,size);
          if(l != -1) {
            close(h);
            size=l; buf.Truncate(offset+size);
            return true;
          };
          free(buf);
        };
      };
      close(h);
    };
  };
  // Can't process this path
  offset=0; size=0;
  return false;
}

}; // namespace ARex

