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
#include <arc/win32.h>
#endif

namespace Paul
{

#ifndef WIN32

// Linux specific
SysInfo::SysInfo(void)
{
    struct utsname u;
    if (uname(&u) == 0) {
        if (strcmp(u.sysname, "Linux") == 0) {
            osFamily = "linux";
        } else {
            osFamily = "unknown";
        }
        osName = "generallinux";
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
            std::string p = Arc::trim(t[1]);
            if (p == "GenuineIntel") {
                platform = "i386";
            } else if (p == "AMD64") { // XXX not sure
                platform = "amd64";
            } else {
                platform = "unknown";
            }
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
            mainMemorySize = Arc::stringtoui(size)/1024;
        }
        if (s.find("SwapTotal:") != std::string::npos) {
            std::vector<std::string> t;
            Arc::tokenize(s, t, ":");
            std::string size = Arc::trim(t[1]);
            swap_size = Arc::stringtoui(size);
        }
    }
    virtualMemorySize = (mainMemorySize + swap_size)/1024;
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

typedef void (WINAPI *PGNSI)(LPSYSTEM_INFO);
typedef BOOL (WINAPI *PGPI)(DWORD, DWORD, DWORD, PDWORD);

// Win32 specific
SysInfo::SysInfo(void)
{
    // get operating system version 
    OSVERSIONINFO os_ver;
    SYSTEM_INFO si;
    PGNSI pGNSI;
    PGPI pGPI;

    ZeroMemory(&os_ver, sizeof(OSVERSIONINFO));
    os_ver.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    ZeroMemory(&si, sizeof(SYSTEM_INFO));

    if (GetVersionEx(&os_ver) == true) {
        osFamily = "windows";
        if (os_ver.dwMajorVersion == 5 && os_ver.dwMinorVersion == 0) {
            osName = "windows2000";
        } else if (os_ver.dwMajorVersion == 5 && os_ver.dwMinorVersion == 1) {
            osName = "windowsxp";
        } else if (os_ver.dwMajorVersion == 5 && os_ver.dwMinorVersion == 2) {
            osName = "windows2003";
        } else if (os_ver.dwMajorVersion == 6) {
            osName = "windowsxp";
        }
        osVersion = os_ver.szCSDVersion;
    }
     
    pGNSI = (PGNSI) GetProcAddress(GetModuleHandle(TEXT("kernel32.dll")), "GetNativeSystemInfo");
    if(pGNSI != NULL) {
        pGNSI(&si);
    } else {
        GetSystemInfo(&si);
    }
    if (si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64) {
        platform = "amd64";
    } else if(si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_INTEL) {
        platform = "i386";
    } else if(si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_IA64) {
        platform = "itanium";
    }
    
    physicalCPUs = si.dwNumberOfProcessors;
    logicalCPUs = si.dwNumberOfProcessors;

    // Get Memory Information
    MEMORYSTATUSEX memory_status;
    memory_status.dwLength = sizeof(memory_status);
    GlobalMemoryStatusEx(&memory_status);
    mainMemorySize = memory_status.ullTotalPhys/(1024*1024);
    virtualMemorySize = memory_status.ullTotalVirtual/(1024*1024);
}

unsigned int SysInfo::diskAvailable(const std::string &path)
{
    ULARGE_INTEGER available_bytes, total_bytes, total_free_bytes;
    GetDiskFreeSpaceEx(path.c_str(), &available_bytes, &total_bytes, &total_free_bytes);
    return (Int64ShraMod32(available_bytes.QuadPart, 10)/1024);
}

unsigned int SysInfo::diskTotal(const std::string &path)
{
    ULARGE_INTEGER available_bytes, total_bytes, total_free_bytes;
    GetDiskFreeSpaceEx(path.c_str(), &available_bytes, &total_bytes, &total_free_bytes);
    return (Int64ShraMod32(total_bytes.QuadPart, 10)/1024);
}

unsigned int SysInfo::diskFree(const std::string &path)
{
    ULARGE_INTEGER available_bytes, total_bytes, total_free_bytes;
    GetDiskFreeSpaceEx(path.c_str(), &available_bytes, &total_bytes, &total_free_bytes);
    return (Int64ShraMod32(total_free_bytes.QuadPart, 10)/1024);
}

void SysInfo::refresh(void)
{
    // NOP
}
#endif

} // namespace Paul
