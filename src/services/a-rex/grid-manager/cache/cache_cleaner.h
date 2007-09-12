#ifndef __ARC_CACHE_CLEANER_H__
#define __ARC_CACHE_CLEANER_H__
#include "../jobs/users.h"

/*
 Functionality: 
    Remove files from cache to fit requrements given in configuration
    file. Also checks for "zombie" files (cache files claimed by 
    non-existing jobs.
 Returns:
    0 - success
*/

int cache_cleaner(const JobUsers &users);

#endif 
