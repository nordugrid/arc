#ifndef __GM_GRIDMAP_H__
#define __GM_GRIDMAP_H__

#include <string>

/*
  Read gridmap file (specified by globus_gridmap global variable).
  Returns:
    true - success
    false - error (most probaly file is missing)
    'ulist' contains unix usernames found in gridmap file separted by
    blank spaces.
*/
bool gridmap_user_list(std::string &ulist);

#endif // __GM_GRIDMAP_H__
