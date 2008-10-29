%{
#include <arc/data/DataStatus.h>
#include <arc/data/FileInfo.h>
#include <arc/data/DataPoint.h>
#include <arc/data/DataHandle.h>
#include <arc/data/DataSpeed.h>
#include <arc/data/DataBuffer.h>
#include <arc/data/FileCache.h>
#include <arc/data/DataMover.h>
%}

#ifdef SWIGPYTHON
namespace Arc {

/* this typemap tells SWIG that we don't want to use the 'std::list<FileInfo>& files' argument
from the target language, but we need a temporary variable for internal use,
and we want this argument to point to this temporary variable */
%typemap(in, numinputs=0) std::list<FileInfo> & files (std::list<FileInfo> temp) {
    $1 = &temp;
}

/* this typemap tells SWIG what we want to do with the 'std::list<FileInfo> & files'
argument after the method finished:
first we create a python list from the std::list (it should be a better way to do this...)
then we want to return a tuple with two members, the first member will be the newly created list,
and the second member is the original return value, the DataStatus. */
%typemap(argout) std::list<FileInfo> & files {
    PyObject *o, *tuple;
    o = PyList_New(0);
    std::list<Arc::FileInfo>::iterator it;
    for (it = (*$1).begin(); it != (*$1).end(); ++it) {
        PyList_Append(o, SWIG_NewPointerObj(&(*it), SWIGTYPE_p_Arc__FileInfo, SWIG_POINTER_OWN | 0 ));
    }
    //o = SWIG_NewPointerObj($1, SWIGTYPE_p_std__listT_Arc__FileInfo_std__allocatorT_Arc__FileInfo_t_t, SWIG_POINTER_OWN | 0 );
    tuple = PyTuple_New(2);
    PyTuple_SetItem(tuple,0,o);
    PyTuple_SetItem(tuple,1,$result);
    $result = tuple;
}


} 
%include "../src/hed/libs/data/DataStatus.h"
%include "../src/hed/libs/data/FileInfo.h"
%include "../src/hed/libs/data/DataPoint.h"
%include "../src/hed/libs/data/DataHandle.h"
%include "../src/hed/libs/data/DataSpeed.h"
%include "../src/hed/libs/data/DataBuffer.h"
%include "../src/hed/libs/data/FileCache.h"
%include "../src/hed/libs/data/DataMover.h"
#endif

