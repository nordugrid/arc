#include <string>

// int canonical_dir(std::string &name);
bool remove_last_name(std::string &name);
bool keep_last_name(std::string &name);
char* remove_head_dir_c(const char* name,int dir_len);
std::string remove_head_dir_s(std::string &name,int dir_len);
const char* get_last_name(const char* name);
 
