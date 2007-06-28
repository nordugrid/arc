#ifndef __GM_CONF_H__
#define __GM_CONF_H__

#include <cstring>
#include <iostream>
#include <fstream>
#include <string>

extern char* config_file;

/*
  Open/assign configuration file to provided cfile.
  Returns:
    true - success
    false - error
*/
bool config_open(std::ifstream &cfile);
/*
  Same as previous, but filename is given by 'name'.
*/
bool config_open(std::ifstream &cfile,std::string &name);
/*
  Closes configuration file. Equal to cfile.close().
  These finctions are provided only for unified interface 
  and for future enhancements.
*/
bool config_close(std::ifstream &cfile);
/*
  Read one line of configuration file.
  Accepts:
    cfile - opened file.
    separator - character used to separate keywords.
  Returns:
    string containing first keyword in line.
    rest - rest of the string.
  Note: use separator=\0 to get unbroken string.
*/
std::string config_read_line(std::istream &cfile);
std::string config_read_line(std::istream &cfile,std::string &rest,char separator = ' ');
/*
  Extract one more keyword from 'rest'.
  Accepts:
   rest - current configuration string.
 Returns:
   string containing first keyword in line.
   rest - rest of the string.
*/
std::string config_next_arg(std::string &rest,char separator = ' ');

#endif // __GM_CONF_H__
