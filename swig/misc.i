%{
#include <arc/misc/ClientInterface.h>
#include <arc/misc/ClientTool.h>
%}

%apply PayloadSOAP ** OUTPUT { PayloadSOAP **response }; 
%include "../src/hed/libs/misc/ClientInterface.h"
%clear PayloadSOAP **response;
%include "../src/hed/libs/misc/ClientTool.h"
