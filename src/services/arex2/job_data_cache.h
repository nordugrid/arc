#ifndef __AREX2_JOB_DATA_CACHE_H__
#define __AREX2_JOB_DATA_CACHE_H__

#include <string>

namespace ARex2
{

/** Data cache */
class JobDataCache
{
    private:
        /* directories to store cache information and cached files */
        std::string cache_dir;
        std::string cache_data_dir;
        /* directory used to create soft-links to cached files */
        std::string cache_link_dir;
        /* if cache is owned ONLY by this user */
        bool private_cache;
        /* upper and lower watermarks for cache */
        long long int cache_max;
        long long int cache_min;
    public: 
        JobDataCache(const std::string &dir,
                     const std::string &data_dir, 
                     bool priv = false);
        JobDataCache(const std::string &dir,
                     const std::string &data_dir,
                     const std::string &link_dir,
                     bool priv = false);
        void SetCacheSize(long long int cache_max,long long int cache_min = 0);
        const std::string & CacheDir(void) const { return cache_dir; };
        const std::string & CacheDataDir(void) const { return cache_data_dir; };
        const std::string & CacheLinkDir(void) const { return cache_link_dir; };
        long long int CacheMaxSize(void) const { return cache_max; };
        long long int CacheMinSize(void) const { return cache_min; };
        bool CachePrivate(void) const { return private_cache; };
};

}; // namespace ARex2

#endif
