%{
#include <arc/client/ClientInterface.h>
%}

%apply PayloadSOAP ** OUTPUT { PayloadSOAP **response }; 
%include "../src/hed/libs/client/ClientInterface.h"
%clear PayloadSOAP **response;
