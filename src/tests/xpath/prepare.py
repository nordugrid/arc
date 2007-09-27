#!/usr/bin/env python
import random
import sys

for i in range(0, int(sys.argv[1])):
    f = open("file" + str(i) + ".xml", "w+") 
    f.write('<?xml version="1.0" encoding="UTF-8"?>')
    f.write("<JobEntry>")
    for j in range(0,int(sys.argv[2])):
        f.write("<AttributeName" + str(j) + ">")
        f.write(str(random.randint(0,10000)))
        f.write("</AttributeName" + str(j) + ">")
    f.write("</JobEntry>")
    f.close()
