#include <iostream>
#include <fstream>
#include <list>
#include <vector>
#include <arc/StringConv.h>
#include "sysinfo.h"

int main(int argc, char **argv)
{
#if 0
    Paul::SysInfo si;

    std::cout << "OSName: " << si.getOSName() << std::endl;
    std::cout << "OSFamily: " << si.getOSFamily() << std::endl;
    std::cout << "OSVersion: " << si.getOSVersion() << std::endl;
    std::cout << "Platform: " << si.getPlatform() << std::endl;
    std::cout << "Pys CPU: " << si.getPhysicalCPUs() << std::endl;
    std::cout << "Log CPU: " << si.getLogicalCPUs() << std::endl;
    std::cout << "Main Memory (MB): " << si.getMainMemorySize() << std::endl;
    std::cout << "Virtual Memory (MB): " << si.getVirtualMemorySize() << std::endl;
    std::cout << "Available Disk space (MB): " << si.diskAvailable(".") << std::endl;
    std::cout << "Total Disk space (MB): " << si.diskTotal(".") << std::endl;
    std::cout << "Free Disk space (MB): " << si.diskFree(".") << std::endl;
#endif
    
    std::ifstream fin(argv[1]);
    int line_n = 20;
    int ln = 0;
    int seek_size = 1024;
    std::list<std::string> lines;
    fin.seekg(0, std::ios::end);
    int block_end = fin.tellg(); 
    fin.seekg(-seek_size, std::ios::end);
    int pos = fin.tellg();
    int block_start = pos;
    std::cout << "bs: " << block_start << std::endl;
    std::cout << "p: " << pos << std::endl;
    std::cout << "be: " << block_end << std::endl;
    std::vector<std::string> tokens;
    char buffer[1024];
    for(;;) { 
        memset(buffer, 0, sizeof(buffer));
        fin.read(buffer, sizeof(buffer));
        buffer[sizeof(buffer)-1] = '\0';
        std::string l = buffer;
        tokens.clear();
        Arc::tokenize(l, tokens, "\n");
        for (int i = tokens.size() -1 ; i >= 0; i--) {
std::cout << "**" << tokens[i] << "(" << i << "/" << tokens.size() << ")" << std::endl;
            lines.push_back(tokens[i]);
            ln++;
        }
        if (ln >= line_n) {
            break;
        }
        pos = fin.tellg();
std::cout << pos << std::endl;
        if ((fin.eof() && ln < line_n) || pos >= block_end) {
            if (block_start == 0) {
                break;
            }
            block_end = block_start;
            int s = block_start - seek_size;
            if (s < 0) {
                s = 0;
            }
            fin.seekg(s, std::ios::beg);
            block_start = fin.tellg();
        }
    std::cout << "bs: " << block_start << std::endl;
    std::cout << "p: " << pos << std::endl;
    std::cout << "be: " << block_end << std::endl;
    }
std::cout << "------" << std::endl;
    std::list<std::string>::iterator it;
    for (it = lines.begin(); it != lines.end(); it++) {
        std::cout << *it << std::endl;
    }
    return 0;
}
