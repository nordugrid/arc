#include <iostream>
#include <arc/data/DataPoint.h>
#include <arc/data/DataHandle.h>
#include <arc/data/DataBuffer.h>

using namespace Arc;

int main(int argc, char** argv) {
  #define DESIRED_SIZE 512
  if (argc != 2) {
    std::cerr<<"Usage: partial_copy filename"<<std::endl;
    return -1;
  }
  Arc::UserConfig usercfg;
  URL url(argv[1]);
  DataHandle handle(url,usercfg);
  if(!handle || !(*handle)) {
    std::cerr<<"Unsupported URL protocol or malformed URL"<<std::endl;
    return -1;
  };
  handle->SetSecure(false); // GridFTP servers generally do not have encrypted data channel
  FileInfo info;
  if(!handle->Stat(info)) {
    std::cerr<<"Failed Stat"<<std::endl;
    return -1;
  };
  unsigned long long int fsize = handle->GetSize();
  if(fsize == (unsigned long long int)-1) {
    std::cerr<<"file size is not available"<<std::endl;
    return -1;
  };
  if(fsize == 0) {
    std::cerr<<"file is empty"<<std::endl;
    return -1;
  };
  if(fsize > DESIRED_SIZE) {
    handle->Range(fsize-DESIRED_SIZE,fsize-1);
  };
  DataBuffer buffer;
  if(!handle->StartReading(buffer)) {
    std::cerr<<"Failed to start reading"<<std::endl;
    return -1;
  };
  for(;;) {
    int n;
    unsigned int length;
    unsigned long long int offset;
    if(!buffer.for_write(n,length,offset,true)) {
      break;
    };
    std::cout<<"BUFFER: "<<offset<<": "<<length<<" :"<<std::string((const char*)(buffer[n]),length)<<std::endl;
    buffer.is_written(n);
  };
  if(buffer.error()) {
    std::cerr<<"Transfer failed"<<std::endl;
  };
  handle->StopReading();
  return 0;
}
