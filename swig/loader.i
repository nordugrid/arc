#ifdef SWIGPYTHON
%module loader

%include "Arc.i"

%import "../src/hed/libs/common/XMLNode.h"
%import "../src/hed/libs/common/Logger.h"
#endif


// Wrap contents of $(top_srcdir)/src/hed/libs/loader/ModuleManager.h
/* The 'operator Glib::Module*' method cannot be wrapped. If it is
 * needed in the bindings, it should be renamed.
 */
%ignore Arc::LoadableModuleDescription::operator Glib::Module*;
%ignore Arc::LoadableModuleDescription::operator!;
%ignore Arc::LoadableModuleDescription::operator=(Glib::Module*);
%ignore Arc::LoadableModuleDescription::operator=(const LoadableModuleDescription&);
%{
#include <arc/loader/ModuleManager.h>
%}
%include "../src/hed/libs/loader/ModuleManager.h"


// Wrap contents of $(top_srcdir)/src/hed/libs/loader/Plugin.h
/* Suppress warnings about potential possibility of memory leak, when
 * using Arc::plugins_table_name, and
 * Arc::PluginDescriptor::{name,kind,description}
 */
%warnfilter(SWIGWARN_TYPEMAP_CHARLEAK) Arc::plugins_table_name;
%warnfilter(SWIGWARN_TYPEMAP_CHARLEAK) Arc::PluginDescriptor;
%{
#include <arc/loader/Plugin.h>
%}
#ifdef SWIGJAVA
/* Make garbage collector destroy all plugin objects _before_ destroying loader */
%typemap(javaout) Arc::Plugin * get_instance {
    long cPtr = $jnicall;
    $javaclassname ret = null;
    if (cPtr != 0) {
      ret = new $javaclassname(cPtr, $owner);
      ret.setMemoryManager(this);
    }
    return ret;
  }
#endif
%include "../src/hed/libs/loader/Plugin.h"
%wraplist(ModuleDesc, Arc::ModuleDesc);
%wraplist(PluginDesc, Arc::PluginDesc);


// Wrap contents of $(top_srcdir)/src/hed/libs/loader/Loader.h
%{
#include <arc/loader/Loader.h>
%}
%include "../src/hed/libs/loader/Loader.h"
