#include <iostream>

#include <arc/XMLNode.h>

// common

void strprintf(std::ostream& out,const char* fmt,
                const std::string& arg1 = "",const std::string& arg2 = "",
                const std::string& arg3 = "",const std::string& arg4 = "",
                const std::string& arg5 = "",const std::string& arg6 = "",
                const std::string& arg7 = "",const std::string& arg8 = "",
                const std::string& arg9 = "",const std::string& arg10 = "");

void strprintf(std::string& out,const char* fmt,
                const std::string& arg1 = "",const std::string& arg2 = "",
                const std::string& arg3 = "",const std::string& arg4 = "",
                const std::string& arg5 = "",const std::string& arg6 = "",
                const std::string& arg7 = "",const std::string& arg8 = "",
                const std::string& arg9 = "",const std::string& arg10 = "");


// simple type

void simpletypeprint(Arc::XMLNode stype,const std::string& ns,std::ostream& h_file,std::ostream& cpp_file);


// complex type

void complextypeprint(Arc::XMLNode ctype,const std::string& ns,std::ostream& h_file,std::ostream& cpp_file);


// entry point

bool schemaconv(Arc::XMLNode wsdl,std::ostream& h_file,std::ostream& cpp_file,const std::string& name);

