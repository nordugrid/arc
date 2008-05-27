#include <string>
#include <iostream>

#include <arc/ArcLocation.h>
#include <arc/IString.h>

#include "OptionParser.h"

int main(int argc, char **argv) {

  Arc::ArcLocation::Init(argv[0]);
  setlocale(LC_ALL, "");

  Arc::OptionParser opts(istring("args..."),
			 istring("This is a test of the OptionParser."),
			 istring("For more information see the header file."));

  bool b = false;
  opts.AddOption('b', "boolean", istring("Boolean option"), b);

  int i = 0;
  opts.AddOption('i', "integer", istring("Integer option"),
		 istring("number"), i);

  std::string s;
  opts.AddOption('s', "string", istring("String option"),
		 istring("word"), s);

  std::list<std::string> l;
  opts.AddOption('l', "listitem", istring("String list option"),
		 istring("word"), l);

  std::list<std::string> args = opts.Parse(argc, argv);

  std::cout << "b: " << b << std::endl;
  std::cout << "i: " << i << std::endl;
  std::cout << "s: " << s << std::endl;

  for (std::list<std::string>::iterator it = l.begin();
       it != l.end(); it++)
    std::cout << "l: " << *it << std::endl;

  for (std::list<std::string>::iterator it = args.begin();
       it != args.end(); it++)
    std::cout << "args: " << *it << std::endl;
}
