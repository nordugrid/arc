#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>
#include <limits.h>

#include <arc/StringConv.h>
#include "../misc/canonical_dir.h"
#include "../misc/escaped.h"
#include "info_types.h"

#if defined __GNUC__ && __GNUC__ >= 3

#define istream_readline(__f,__s,__n) {      \
   __f.get(__s,__n,__f.widen('\n'));         \
   if(__f.fail()) __f.clear();               \
   __f.ignore(std::numeric_limits<std::streamsize>::max(), __f.widen('\n')); \
}

#else

#define istream_readline(__f,__s,__n) {      \
   __f.get(__s,__n,'\n');         \
   if(__f.fail()) __f.clear();               \
   __f.ignore(INT_MAX,'\n'); \
}

#endif

static Arc::Logger& logger = Arc::Logger::getRootLogger();

void output_escaped_string(std::ostream &o,const std::string &str) {
  std::string::size_type n,nn;
  for(nn=0;;) {
//    if((n=str.find(' ',nn)) == std::string::npos) break;
    if((n=str.find_first_of(" \\",nn)) == std::string::npos) break;
    o.write(str.data()+nn,n-nn); 
    o.put('\\');
    o.put(*(str.data()+n));
    nn=n+1;
  };
  o.write(str.data()+nn,str.length()-nn);
}

inline void fix_range(int &n,int max,int min = 0) {
  if(n>max) n=max;
  if(n<min) n=min;
}

std::string mds_time::str(void) const {
  struct tm  t_buf;
  struct tm* t_;
  time_t tt = t;
  char buf[16];
  if(tt != -1) {
    t_=gmtime_r(&tt,&t_buf);
    t_->tm_year+=1900;
    fix_range(t_->tm_year,9999);
    fix_range(t_->tm_mon,99); t_->tm_mon++;
    fix_range(t_->tm_mday,99);
    fix_range(t_->tm_hour,99);
    fix_range(t_->tm_min,99);
    fix_range(t_->tm_sec,99);
    sprintf(buf,"%04u%02u%02u%02u%02u%02uZ",
       t_->tm_year,t_->tm_mon,t_->tm_mday,t_->tm_hour,t_->tm_min,t_->tm_sec);
  }
  else {
//    strcpy(buf,"00000000000000Z");
    buf[0]=0;
  };
  return std::string(buf);
}

std::ostream &operator<< (std::ostream &o,const mds_time &t) {
  o<<t.str();
  return o;
}

inline bool get_num(const std::string &s,int pos,int len,unsigned int &num) {
  if(pos < 0) { len+=pos; pos=0; };
  if(len <= 0) return false;
  if(!Arc::stringto(s.substr(pos,len),num)) return false;
  return true;
}

mds_time& mds_time::operator= (const char* s) {
  std::string s_(s);
  return operator= (s_);
}

mds_time& mds_time::operator= (std::string s) {
  struct tm tt;
  t = (time_t)(-1);
  int pos = s.length()-1;
  if(pos<0) return (*this);
  if(s[pos] == 'Z') pos--; pos--;
  unsigned int value;
  if(!get_num(s,pos,2,value)) return (*this); tt.tm_sec=value; pos-=2;
  if(!get_num(s,pos,2,value)) return (*this); tt.tm_min=value; pos-=2;
  if(!get_num(s,pos,2,value)) return (*this); tt.tm_hour=value; pos-=2;
  if(!get_num(s,pos,2,value)) return (*this); tt.tm_mday=value; pos-=2;
  if(!get_num(s,pos,2,value)) return (*this); tt.tm_mon=value-1; pos-=4;
  if(!get_num(s,pos,4,value)) return (*this); tt.tm_year=value;
  tt.tm_isdst=-1; tt.tm_year-=1900;
  t = mktime(&tt);
  if(t == -1) return (*this);
  /* problems with timezone - using more complex method */
  long int timezone_;
  struct tm  tt_buf;
  struct tm* tt_;
  time_t t_;
  tt_=gmtime_r(&t,&tt_buf);
  tt_->tm_isdst=-1; t_ = mktime(tt_); timezone_=t-t_;
  t+=timezone_;
  return (*this);
}

std::istream &operator>> (std::istream &i,mds_time &t) {
  char buf[1024];
  istream_readline(i,buf,sizeof(buf));
  std::string s = buf;
  t=s;
  return i;
}

std::ostream &operator<< (std::ostream &o,const FileData &fd) {
  output_escaped_string(o,fd.pfn);
  o.put(' ');
  output_escaped_string(o,fd.lfn);
  return o;
}

std::istream &operator>> (std::istream &i,FileData &fd) {
  char buf[1024];
  istream_readline(i,buf,sizeof(buf));
  fd.pfn.resize(0); fd.lfn.resize(0);
  int n=input_escaped_string(buf,fd.pfn);
  input_escaped_string(buf+n,fd.lfn);
  if((fd.pfn.length() == 0) && (fd.lfn.length() == 0)) return i; /* empty st */
  if(canonical_dir(fd.pfn) != 0) {
    logger.msg(Arc::ERROR,"Wrong directory in %s",buf);
    fd.pfn.resize(0); fd.lfn.resize(0);
  };
  return i;
}

FileData::FileData(void) {
}

FileData::FileData(const char *pfn_s,const char *lfn_s) {
  if(pfn_s) { pfn=pfn_s; } else { pfn.resize(0); };
  if(lfn_s) { lfn=lfn_s; } else { lfn.resize(0); };
}

FileData& FileData::operator= (const char *str) {
  pfn.resize(0); lfn.resize(0);
  int n=input_escaped_string(str,pfn);
  input_escaped_string(str+n,lfn);
  return *this; 
}

bool FileData::operator== (const FileData& data) {
  // pfn may contain leading slash. It must be striped
  // before comparison.
  const char* pfn_ = pfn.c_str();
  if(pfn_[0] == '/') ++pfn_;
  const char* dpfn_ = data.pfn.c_str();
  if(dpfn_[0] == '/') ++dpfn_;
  return (strcmp(pfn_,dpfn_) == 0);
  // return (pfn == data.pfn);
}

bool FileData::operator== (const char *name) {
  if(name == NULL) return false;
  if(name[0] == '/') ++name;
  const char* pfn_ = pfn.c_str();
  if(pfn_[0] == '/') ++pfn_;
  return (strcmp(pfn_,name) == 0);
}

bool FileData::has_lfn(void) {
  return (lfn.find(':') != std::string::npos);
}

bool LRMSResult::set(const char* s) {
  // 1. Empty string = exit code 0
  if(s == NULL) s="";
  for(;*s;++s) { if(!isspace(*s)) break; };
  if(!*s) { code_=0; description_=""; };
  // Try to read first word as number
  char* e;
  code_=strtol(s,&e,0);
  if((!*e) || (isspace(*e))) { 
    for(;*e;++e) { if(!isspace(*e)) break; };
    description_=e;
    return true;
  };
  // If there is no number that means some "uncoded" failure
  code_=-1;
  description_=s;
  return true;
}

std::istream& operator>>(std::istream& i,LRMSResult &r) {
  char buf[1025]; // description must have reasonable size
  if(i.eof()) { buf[0]=0; } else { istream_readline(i,buf,sizeof(buf)); };
  r=buf;
  return i;  
}

std::ostream& operator<<(std::ostream& o,const LRMSResult &r) {
  o<<r.code()<<" "<<r.description();
  return o;
}

