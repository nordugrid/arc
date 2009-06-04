import os
import time, random
import commands
memory= [] 
#f = open('memory_Bartender_24.txt', 'w')
while 1:

    time.sleep(0.5)
    #os.system('ps -C arched -o vsz,rss -o args >> memory_Cache_Bartender_sal1_client90_1.txt')
    #os.system('cat /proc/26141/statm >> memory_Cache_Bartender_sal1_client90_1.txt')
    #os.system('pmap -x 26141 >> memory_Cache_Bartender_sal1_client90.txt')
    #os.system('cat /proc/26141/statm')
    output = commands.getstatusoutput('ps -C arched -o vsz,rss -o args')
    t = output[1].find('\n')
    print output[1][t+1:]
    #f = open('memory_Cache_Bartender_alone_sal1_client1.txt', 'w')
    #f.write(output[1][t+1:]+'\n')
#f.close()
