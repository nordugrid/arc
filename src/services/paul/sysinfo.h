#ifndef __PAUL_SYSINFO_H__
#define __PAUL_SYSINFO_H__

#include <string>

namespace Paul
{

class SysInfo
{
    private:
        // static infos geathered by initialization
        std::string osFamily;
        std::string osName;
        std::string osVersion;
        std::string platform;
        unsigned int physicalCPUs;
        unsigned int logicalCPUs;
        unsigned int mainMemorySize;
        unsigned int virtualMemorySize;
    public:
        SysInfo(void);
        void refresh(void);
        const std::string &getOSName(void) { return osName; };
        const std::string &getOSFamily(void) { return osFamily; };
        const std::string &getOSVersion(void) { return osVersion; };
        const std::string &getPlatform(void) { return platform; };
        unsigned int getPhysicalCPUs(void) { return physicalCPUs; };
        unsigned int getLogicalCPUs(void) { return logicalCPUs; };
        unsigned int getMainMemorySize(void) { return mainMemorySize; };
        unsigned int getVirtualMemorySize(void) { return virtualMemorySize; };
        static unsigned int diskAvailable(const std::string &path);
        static unsigned int diskFree(const std::string &path);
        static unsigned int diskTotal(const std::string &path);

};

}

#endif // __PAUL_SYSINFO_H__
