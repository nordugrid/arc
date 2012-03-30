// Wrap contents of <arc/loader/ModuleManager.h>
/* The 'operator Glib::Module*' method cannot be wrapped. If it is
 * needed in the bindings, it should be renamed.
 */
%ignore Arc::LoadableModuleDescription::operator Glib::Module*;
%{
#include <arc/loader/ModuleManager.h>
%}
%include "../src/hed/libs/loader/ModuleManager.h"


// Wrap contents of <arc/loader/Plugin.h>
/* Suppress warnings about potential possibility of memory leak, when
 * using Arc::plugins_table_name, and
 * Arc::PluginDescriptor::{name,kind,description}
 */
%warnfilter(SWIGWARN_TYPEMAP_CHARLEAK) Arc::plugins_table_name;
%warnfilter(SWIGWARN_TYPEMAP_CHARLEAK) Arc::PluginDescriptor;
%{
#include <arc/loader/Plugin.h>
%}
%include "../src/hed/libs/loader/Plugin.h"


// Wrap contents of <arc/loader/Loader.h>
%{
#include <arc/loader/Loader.h>
%}
%include "../src/hed/libs/loader/Loader.h"
