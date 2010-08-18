/**
 * Note that the order of the "%include" statements are important! If a
 * "%include" depends on other "%include"s, it should be placed after these
 * "%include" dependencies.
 */

%{
#include <arc/credential/Credential.h>
%}

#define STACK_OF(A) void
%include "../src/hed/libs/credential/Credential.h"
