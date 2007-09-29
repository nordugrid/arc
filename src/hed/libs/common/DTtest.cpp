#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "BasicDT.h"
#include <iostream>
#include "DateTime.h"
#include "Logger.h"

int main(void)
{  
   Arc::LogStream cerr(std::cerr);
   Arc::Logger::getRootLogger().addDestination(cerr);

  //Test class DateTime
   Arc::Time now;
   Arc::DateTime* datetime = new Arc::DateTime();
   std::string strtmp = datetime->serialize(&now);

   std::cout << strtmp << std::endl;

   Arc::Time *tmp1 = datetime->deserialize(strtmp.c_str());
   std::string strtmp1 = datetime->serialize(tmp1);

   std::cout << strtmp1 << std::endl;

  //Test class Duration
   Arc::Period p(123456789);
   Arc::Duration* duration = new Arc::Duration();
   std::string strtmp2 = duration->serialize(&p);

   std::cout << strtmp2 << std::endl;

   Arc::Period *tmp2 = duration->deserialize(strtmp2.c_str());
   std::string strtmp3 = duration->serialize(tmp2);

   std::cout << strtmp3 << std::endl;
}
