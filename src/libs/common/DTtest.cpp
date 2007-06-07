#include "BasicDT.h"
#include <iostream>
#include <time.h>

int main(void)
{  
//Test class DateTime
   time_t now = time(NULL);
   struct tm *tmp = gmtime(&now);
   Arc::DateTime* datetime = new Arc::DateTime();
   std::string strtmp = datetime->serialize(tmp);
    
   std::cout << strtmp << std::endl;
   
   

   struct tm *tmp1 = datetime->deserialize(strtmp.c_str());
   std::string strtmp1 = datetime->serialize(tmp1);

   std::cout << strtmp1 << std::endl;


}
