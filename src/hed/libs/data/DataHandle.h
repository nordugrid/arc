// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_DATAHANDLE_H__
#define __ARC_DATAHANDLE_H__

#include <arc/data/DataPoint.h>

namespace Arc {

  class URL;
  class UserConfig;

  /// This class is a wrapper around the DataPoint class.
  /** It simplifies the construction, use and destruction of
   * DataPoint objects and should be used instead of DataPoint
   * classes directly. The appropriate DataPoint subclass is
   * created automatically and stored internally in DataHandle.
   * A DataHandle instance can be thought of as a pointer to
   * the DataPoint instance and the DataPoint can be accessed
   * through the usual dereference operators. A DataHandle
   * cannot be copied.
   *
   * This class is main way to access remote data items and
   * obtain information about them. Below is an example of
   * accessing last 512 bytes of files stored at GridFTP
   * server. To simply copy a whole file DataMover::Transfer() can
   * be used.
   *
   * \code
   * #include <iostream>
   * #include <arc/data/DataPoint.h>
   * #include <arc/data/DataHandle.h>
   * #include <arc/data/DataBuffer.h>
   * 
   * using namespace Arc;
   * 
   * int main(void) {
   *   #define DESIRED_SIZE 512
   *   Arc::UserConfig usercfg;
   *   URL url("gsiftp://localhost/files/file_test_21");
   *   DataPoint* handle = DataHandle::GetPoint(url,usercfg);
   *   if(!handle) {
   *     std::cerr<<"Unsupported URL protocol or malformed URL"<<std::endl;
   *     return -1;
   *   };
   *   FileInfo info;
   *   if(!handle->Stat(info)) {
   *     std::cerr<<"Failed Stat"<<std::endl;
   *     return -1;
   *   };
   *   unsigned long long int fsize = handle->GetSize();
   *   if(fsize == (unsigned long long int)-1) {
   *     std::cerr<<"file size is not available"<<std::endl;
   *     return -1;
   *   };
   *   if(fsize == 0) {
   *     std::cerr<<"file is empty"<<std::endl;
   *     return -1;
   *   };
   *   unsigned long long int foffset;
   *   if(fsize > DESIRED_SIZE) {
   *     handle->Range(fsize-DESIRED_SIZE,fsize-1);
   *   };
   *   unsigned int wto;
   *   DataBuffer buffer;
   *   if(!handle->PrepareReading(10,wto)) {
   *     std::cerr<<"Failed PrepareReading"<<std::endl;
   *     return -1;
   *   };
   *   if(!handle->StartReading(buffer)) {
   *     std::cerr<<"Failed StopReading"<<std::endl;
   *     return -1;
   *   };
   *   for(;;) {
   *     int n;
   *     unsigned int length;
   *     unsigned long long int offset;
   *     if(!buffer.for_write(n,length,offset,true)) {
   *       break;
   *     };
   *     std::cout<<"BUFFER: "<<offset<<": "<<length<<" :"<<std::string((const char*)(buffer[n]),length)<<std::endl;
   *     buffer.is_written(n);
   *   };
   *   if(buffer.error()) {
   *     std::cerr<<"Transfer failed"<<std::endl;
   *   };
   *   handle->StopReading();
   *   handle->FinishReading();
   *   return 0;
   * }
   * \endcode
   * And the same example in python
   * \code
   * import arc
   * 
   * desired_size = 512
   * usercfg = arc.UserConfig()
   * url = arc.URL("gsiftp://localhost/files/file_test_21")
   * handle = arc.DataHandle.GetPoint(url,usercfg)
   * info = arc.FileInfo("")
   * handle.Stat(info)
   * print "Name: ", info.GetName()
   * fsize = info.GetSize()
   * if fsize > desired_size:
   *     handle.Range(fsize-desired_size,fsize-1)
   * buffer = arc.DataBuffer()
   * res, wto = handle.PrepareReading(10)
   * handle.StartReading(buffer)
   * while True:
   *     n = 0
   *     length = 0
   *     offset = 0
   *     ( r, n, length, offset, buf) = buffer.for_write(True)
   *     if not r: break
   *     print "BUFFER: ", offset, ": ", length, " :", buf
   *     buffer.is_written(n);
   * \endcode
   */

  class DataHandle {
  public:
    /// Construct a new DataHandle
    DataHandle(const URL& url, const UserConfig& usercfg)
      : p(getLoader().load(url, usercfg)) {}
    /// Destructor
    ~DataHandle() {
      if (p)
        delete p;
    }
    /// Returns a pointer to a DataPoint object
    DataPoint* operator->() {
      return p;
    }
    /// Returns a const pointer to a DataPoint object
    const DataPoint* operator->() const {
      return p;
    }
    /// Returns a reference to a DataPoint object
    DataPoint& operator*() {
      return *p;
    }
    /// Returns a const reference to a DataPoint object
    const DataPoint& operator*() const {
      return *p;
    }
    /// Returns true if the DataHandle is not valid
    bool operator!() const {
      return !p;
    }
    /// Returns true if the DataHandle is valid
    operator bool() const {
      return !!p;
    }
    /// Returns a pointer to new DataPoint object corresponding to URL.
    /// This static method is mostly for bindings to other languages
    /// and if availability scope of obtained DataPoint is undefined.
    static DataPoint* GetPoint(const URL& url, const UserConfig& usercfg) {
      return getLoader().load(url, usercfg);
    }
  private:
    /// Pointer to specific DataPoint instance
    DataPoint *p;
    /// Returns DataPointLoader object to be used for creating DataPoint objects.
    static DataPointLoader& getLoader();
    /// Private default constructor
    DataHandle(void);
    /// Private copy constructor and assignment operator because DataHandle
    /// should not be copied.
    DataHandle(const DataHandle&);
    DataHandle& operator=(const DataHandle&);
  };

} // namespace Arc

#endif // __ARC_DATAHANDLE_H__
