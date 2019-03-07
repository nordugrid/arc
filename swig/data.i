#ifdef SWIGPYTHON
%module data

%include "Arc.i"

%import "../src/hed/libs/common/CheckSum.h"
%import "../src/hed/libs/common/DateTime.h"
%import "../src/hed/libs/common/URL.h"
%import "../src/hed/libs/common/UserConfig.h"
#endif


// Wrap contents of $(top_srcdir)/src/hed/libs/data/DataStatus.h
%{
#include <arc/data/DataStatus.h>
%}
%ignore Arc::DataStatus::operator!;
%ignore Arc::DataStatus::operator=(const DataStatusType&);
%ignore operator<<(std::ostream&, const DataStatus&);
%include "../src/hed/libs/data/DataStatus.h"


// Wrap contents of $(top_srcdir)/src/hed/libs/data/FileInfo.h
%{
#include <arc/data/FileInfo.h>
%}
%ignore Arc::FileInfo::operator!;
%include "../src/hed/libs/data/FileInfo.h"


// Wrap contents of $(top_srcdir)/src/hed/libs/data/URLMap.h
%{
#include <arc/data/URLMap.h>
%}
%ignore Arc::URLMap::operator!;
%include "../src/hed/libs/data/URLMap.h"


// Wrap contents of $(top_srcdir)/src/hed/libs/data/DataPoint.h
%{
#include <arc/data/DataPoint.h>
%}
%ignore Arc::DataPoint::operator!;
%ignore Arc::DataPointPluginArgument::operator const URL&; // works with swig 1.3.40, and higher...
%ignore Arc::DataPointPluginArgument::operator const Arc::URL&; // works with swig 1.3.29
%ignore Arc::DataPointPluginArgument::operator const UserConfig&; // works with swig 1.3.40, and higher...
%ignore Arc::DataPointPluginArgument::operator const Arc::UserConfig&; // works with swig 1.3.29
#ifdef SWIGPYTHON
/* This typemap tells SWIG that we don't want to use the
 * 'std::list<FileInfo>& files' argument from the target language, but
 * we need a temporary variable for internal use, and we want this
 * argument to point to this temporary variable
 */
%typemap(in, numinputs=0) std::list<Arc::FileInfo>& files (std::list<Arc::FileInfo> temp) { $1 = &temp; }
/* This typemap tells SWIG what we want to do with the
 * 'std::list<FileInfo> & files' argument after the method finished:
 * first we create a python list from the std::list (it should be a
 * better way to do this...) then we want to return a tuple with two
 * members, the first member will be the newly created list, and the
 * second member is the original return value, the DataStatus.
 */
%typemap(argout) std::list<Arc::FileInfo>& files {
  PyObject *o = PyList_New(0);
  std::list<Arc::FileInfo>::iterator it;
  for (it = (*$1).begin(); it != (*$1).end(); ++it) {
    PyList_Append(o, SWIG_NewPointerObj(new Arc::FileInfo(*it), SWIGTYPE_p_Arc__FileInfo, SWIG_POINTER_OWN | 0 ));
  }
  $result = PyTuple_Pack(2, o, $result);
} /* applies to:
 * virtual DataStatus DataPoint::Stat(std::list<FileInfo>& files, const std::list<DataPoint*>& urls, DataPointInfoType verb = INFO_TYPE_ALL)
 * virtual DataStatus DataPoint::List(std::list<FileInfo>& files, DataPointInfoType verb = INFO_TYPE_ALL)
 */
#endif
%include "../src/hed/libs/data/DataPoint.h"
#ifdef SWIGPYTHON
%clear std::list<Arc::FileInfo>& files;
#endif
%wraplist(FileInfo, Arc::FileInfo);
%wraplist(DataPoint, Arc::DataPoint*);


// Wrap contents of $(top_srcdir)/src/hed/libs/data/DataHandle.h
%{
#include <arc/data/DataHandle.h>
%}
%ignore Arc::DataHandle::operator!;
%ignore Arc::DataHandle::operator->;
%include "../src/hed/libs/data/DataHandle.h"


// Wrap contents of $(top_srcdir)/src/hed/libs/data/DataSpeed.h
%{
#include <arc/data/DataSpeed.h>
%}
%include "../src/hed/libs/data/DataSpeed.h"


// Wrap contents of $(top_srcdir)/src/hed/libs/data/DataBuffer.h
%{
#include <arc/data/DataBuffer.h>
%}
%ignore Arc::DataBuffer::operator[](int);
#ifdef SWIGPYTHON
%{
namespace Arc {
typedef struct {
    bool result;
    int handle;
    unsigned int size;
    unsigned long long int offset;
    char* buffer;
} DataBufferForWriteResult;

typedef struct {
    bool result;
    int handle;
    unsigned int size;
} DataBufferForReadResult;
} // namespace Arc
%}

%typemap(out) Arc::DataBufferForWriteResult {
  $result = PyTuple_New(5);
  PyTuple_SetItem($result,0,PyInt_FromLong($1.result));
  PyTuple_SetItem($result,1,PyInt_FromLong($1.handle));
  PyTuple_SetItem($result,2,PyInt_FromLong($1.size));
  PyTuple_SetItem($result,3,PyInt_FromLong($1.offset));
%#if PY_VERSION_HEX>=0x03000000
  PyTuple_SetItem($result,4,$1.buffer?PyUnicode_FromStringAndSize($1.buffer,$1.size):Py_None);
%#else
  PyTuple_SetItem($result,4,$1.buffer?PyString_FromStringAndSize($1.buffer,$1.size):Py_None);
%#endif
}

%typemap(out) Arc::DataBufferForReadResult {
  $result = PyTuple_Pack(3, PyInt_FromLong($1.result), PyInt_FromLong($1.handle), PyInt_FromLong($1.size));
}

%typemap(in) (char* DataBufferIsReadBuf, unsigned int DataBufferIsReadSize) {
%#if PY_VERSION_HEX>=0x03000000
  $input = PyUnicode_AsUTF8String($input);
  $1 = PyBytes_AsString($input);
  $2 = ($1)?PyBytes_Size($input):0;
%#else
  $1 = PyString_AsString($input);
  $2 = ($1)?PyString_Size($input):0;
%#endif
}

%extend Arc::DataBuffer {
    Arc::DataBufferForWriteResult for_write(bool wait) {
        Arc::DataBufferForWriteResult r;
        r.result = self->for_write(r.handle, r.size, r.offset, wait);
        r.buffer = r.result?(self->operator[](r.handle)):NULL;
        return r;
    }
    
    Arc::DataBufferForReadResult for_read(bool wait) {
        Arc::DataBufferForReadResult r;
        r.result = self->for_read(r.handle, r.size, wait);
        return r;
    }

    bool is_read(int handle, char* DataBufferIsReadBuf, unsigned int DataBufferIsReadSize, unsigned long long int offset) {
        char* buf = self->operator[](handle);
        if(!buf) return false;
        if(DataBufferIsReadSize > self->buffer_size()) return false;
        memcpy(buf, DataBufferIsReadBuf, DataBufferIsReadSize);
        return self->is_read(handle, DataBufferIsReadSize, offset);
    }
};

%ignore Arc::DataBuffer::for_write(int&,unsigned int&,unsigned long long int&,bool);
%ignore Arc::DataBuffer::for_read(int&,unsigned int&,bool);
%ignore Arc::DataBuffer::is_read(int,unsigned int,unsigned long long int);
%ignore Arc::DataBuffer::is_read(char*,unsigned int,unsigned long long int);
%ignore Arc::DataBuffer::is_written(char*);
%ignore Arc::DataBuffer::is_notwritten(char*);
#endif
%include "../src/hed/libs/data/DataBuffer.h"


// Wrap contents of $(top_srcdir)/src/hed/libs/data/FileCache.h
typedef int uid_t;
typedef int gid_t;
%{
#include <arc/data/FileCache.h>
%}
%include "../src/hed/libs/data/FileCache.h"


// Wrap contents of $(top_srcdir)/src/hed/libs/data/DataMover.h
%{
#include <arc/data/DataMover.h>
%}
%include "../src/hed/libs/data/DataMover.h"


// Wrap contents of $(top_srcdir)/src/hed/libs/data-staging/DTRStatus.h
%{
#include <arc/data-staging/DTRStatus.h>
%}
%ignore DataStaging::DTRStatus::operator=(const DTRStatusType&);
%ignore DataStaging::DTRErrorStatus::operator=(const DTRErrorStatusType&);
%include "../src/libs/data-staging/DTRStatus.h"
%template(DTRStatusTypeVector) std::vector<DataStaging::DTRStatus::DTRStatusType>;  


// Wrap contents of $(top_srcdir)/src/hed/libs/data-staging/DTR.h
%{
#include <arc/data-staging/DTR.h>
%}
%ignore DataStaging::DTR::operator!;
%include "../src/libs/data-staging/DTR.h"
%wraplist(DTRCallback, DataStaging::DTRCallback*);


// Wrap contents of $(top_srcdir)/src/hed/libs/data-staging/TransferShares.h
%{
#include <arc/data-staging/TransferShares.h>
%}
%include "../src/libs/data-staging/TransferShares.h"
%template(StringIntMap) std::map<std::string, int>;


// Wrap contents of $(top_srcdir)/src/hed/libs/data-staging/Scheduler.h
%{
#include <arc/data-staging/Scheduler.h>
%}
%include "../src/libs/data-staging/Scheduler.h"


%ignore Arc::ThreadedPointer::operator=(const ThreadedPointer<T>&);
%ignore Arc::ThreadedPointer::operator=(T*);
%ignore Arc::ThreadedPointer::operator!;
%import "../src/hed/libs/common/Thread.h"
%ignore Arc::ThreadedPointer<DataStaging::DTR>::operator bool; // Clash between "operator bool" in DTR and ThreadedPointer (smart pointer wrapping).
%template(DTRPointer) Arc::ThreadedPointer<DataStaging::DTR>;
%template(DTRLogger) Arc::ThreadedPointer<Arc::Logger>;
#ifdef SWIGPYTHON
%pythoncode %{
import arc

def datapoint_from_url(url_string, usercfg=arc.UserConfig()):
    url = arc.URL(url_string)
    data_handle = DataHandle(url, usercfg)
    data_point = data_handle.__ref__()
    url.thisown = False
    data_handle.thisown = False
    return data_point
%}
#endif
