#include <string>
#include <iostream>
#include <ctime> 
#include <cstdlib>
#include "job.h"
#include <glibmm.h>
#include "grid_sched.h"

namespace GridScheduler {

unsigned long random_init = 0;

//UUID generator

std::string generate(void) {
    std::string uuid_str("");
    int rnd[10];
    char buffer[20];
    unsigned long uuid_part;
    for (int j =0; j < 4; j++) {
        for (int i =0; i < 16;i++) {
            rnd[i] = std::rand() % 256;
        }
        rnd[6] = (rnd[6] & 0x4F) | 0x40;
        rnd[8] = (rnd[8] & 0xBF) | 0x80;
        uuid_part= 0L;
        for (int i =0; i < 16;i++) {
            uuid_part = (uuid_part << 8) + rnd[i];
        }
        sprintf(buffer, "%08x", uuid_part);
        uuid_str.append(buffer, 8);
    }
    
    uuid_str.insert(8,"-");
    uuid_str.insert(13,"-");
    uuid_str.insert(18,"-");
    uuid_str.insert(23,"-");
    return uuid_str;
}

// TODO: better random generation
//
//  1 second dependency
//

std::string make_uuid() {
    srand((unsigned)time(NULL) + random_init);
    std::string uuid = generate();
    random_init++;
    return uuid;
}

int main (void) {
    for (int a=0;a<100000; a++ ) {
        std::string uuid = make_uuid();
        std::cout << uuid.c_str() << std::endl;
    }

    return 0;
}


} //namespace

