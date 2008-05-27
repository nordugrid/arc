#include <iostream>
#include "sysinfo.h"

int main(void)
{
    Paul::SysInfo si;

    std::cout << "OSName: " << si.getOSName() << std::endl;
    std::cout << "OSRelease: " << si.getOSRelease() << std::endl;
    std::cout << "OSVersion: " << si.getOSVersion() << std::endl;
    std::cout << "Platform: " << si.getPlatform() << std::endl;
    std::cout << "Pys CPU: " << si.getPhysicalCPUs() << std::endl;
    std::cout << "Log CPU: " << si.getLogicalCPUs() << std::endl;
    std::cout << "Main Memory (MB): " << si.getMainMemorySize() << std::endl;
    std::cout << "Virtual Memory (MB): " << si.getVirtualMemorySize() << std::endl;
    std::cout << "Available Disk space (MB): " << si.diskAvailable(".") << std::endl;
    std::cout << "Total Disk space (MB): " << si.diskTotal(".") << std::endl;
    std::cout << "Free Disk space (MB): " << si.diskFree(".") << std::endl;
    return 0;
}
