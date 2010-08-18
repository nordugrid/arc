/**
 * Note that the order of the "%include" statements are important! If a
 * "%include" depends on other "%include"s, it should be placed after these
 * "%include" dependencies.
 */

%{
#include <arc/loader/Loader.h>
%}

%include "../src/hed/libs/loader/Loader.h"
