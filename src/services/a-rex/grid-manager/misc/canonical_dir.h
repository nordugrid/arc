#include <string>

/*
  Removes /../ from 'name'. If leading_slash=true '/' will be added
  at the beginning of 'name' if missing. Otherwise it will be removed.
  Returns:
    0 - success
    1 - failure, if impossible to remove all /../
  todo: move to bool return type (??).
*/
int canonical_dir(std::string &name,bool leading_slash = true);
