%{
#include <arc/misc/ClientInterface.h>
%}

%apply PayloadSOAP ** OUTPUT { PayloadSOAP **response }; 
%include "../src/hed/libs/misc/ClientInterface.h"
%clear PayloadSOAP **response;
