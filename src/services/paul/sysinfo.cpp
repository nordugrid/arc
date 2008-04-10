#ifdef HAVE_CONFIG_H
#include <config.h>
#endif


#include <iostream>
#include <fstream>
#include <vector>
#include <arc/StringConv.h>

#include "sysinfo.h"
#ifndef WIN32
#include <sys/utsname.h> // uname
#include "fsusage.h"
#else
#endif

namespace Paul
{

#ifndef WIN32

// Linux specific
SysInfo::SysInfo(void)
{
    struct utsname u;
    if (uname(&u) == 0) {
        osName = u.sysname;
        osRelease = u.release;
        osVersion = u.version;
    }
    
    std::ifstream p("/proc/cpuinfo");
    std::string s;
    unsigned int p_num = 0;
    while (getline(p, s)) {
        // found processor line
        if (s.find("processor", 0) != std::string::npos) {
            p_num++;
        }
        if ((s.find("vendor_id", 0) != std::string::npos) && platform.empty()) {
            std::vector<std::string> t;
            Arc::tokenize(s, t, ":");
            platform = Arc::trim(t[1]);
        }
    }
    p.close();
    physicalCPUs = p_num;
    logicalCPUs = p_num;

    std::ifstream m("/proc/meminfo");
    unsigned int swap_size = 0;
    while (getline(m, s)) {
        if (s.find("MemTotal:") != std::string::npos) {
            std::vector<std::string> t;
            Arc::tokenize(s, t, ":");
            std::string size = Arc::trim(t[1]);
            mainMemorySize = Arc::stringtoui(size);
        }
        if (s.find("SwapTotal:") != std::string::npos) {
            std::vector<std::string> t;
            Arc::tokenize(s, t, ":");
            std::string size = Arc::trim(t[1]);
            swap_size = Arc::stringtoui(size);
        }
    }
    virtualMemorySize = mainMemorySize + swap_size;
    m.close();
}

unsigned int SysInfo::diskAvailable(const std::string &path)
{
    struct fs_usage fsu;
    int ret;
    ret = get_fs_usage(path.c_str(), NULL, &fsu);
    if (ret == 0) {
        return (fsu.fsu_blocksize * fsu.fsu_bavail)/(1024*1024);
    }
    return 0;
}

unsigned int SysInfo::diskTotal(const std::string &path)
{
    struct fs_usage fsu;
    int ret;
    ret = get_fs_usage(path.c_str(), NULL, &fsu);
    if (ret == 0) {
        return (fsu.fsu_blocksize * fsu.fsu_blocks)/(1024*1024);
    }
    return 0;
}

unsigned int SysInfo::diskFree(const std::string &path)
{
    struct fs_usage fsu;
    int ret;
    ret = get_fs_usage(path.c_str(), NULL, &fsu);
    if (ret == 0) {
        return (fsu.fsu_blocksize * fsu.fsu_bfree)/(1024*1024);
    }
    return 0;
}

void SysInfo::refresh(void)
{
    // NOP
}
#else
// Win32 specific
SysInfo::SysInfo(void)
{
    osName = "Win32";
    osRelease = "x";
    osVersion = "y";
    platform = "intel";
    physicalCPUs = 1;
    logicalCPUs = 1;
    mainMemorySize = 1;
    virtualMemorySize = 1;
}

unsigned int SysInfo::diskAvailable(const std::string &path)
{
    return 0;
}

unsigned int SysInfo::diskTotal(const std::string &path)
{
    return 0;
}

unsigned int SysInfo::diskFree(const std::string &path)
{
    return 0;
}

void SysInfo::refresh(void)
{
    // NOP
}
#endif

} // namespace Paul
