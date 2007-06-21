#!/usr/bin/python

from platform import node
from sys import argv
from os import popen
from re import *

def matlabRange(threads):
    limits = threads.split(":")
    if len(limits)==1:
        return range(int(limits[0]),int(limits[0])+1)
    if len(limits)==2:
        return range(int(limits[0]),int(limits[1])+1)
    elif len(limits)==3:
        return range(int(limits[0]),int(limits[2])+1,int(limits[1]))
    else:
        print "Badly formed range expression!"
        exit()

if (len(argv)!=5):
    print "Wrong number of arguments!"
    exit()
    
host         = argv[1]
port         = int(argv[2])
threadRange  = matlabRange(argv[3])
duration     = int(argv[4])

numberOfRequestsPattern = compile(r"Number of requests: (\d+)")
completedRequestsPattern = compile(r"Completed requests: (\d+)")
failedRequestsPattern = compile(r"Failed requests: (\d+)")
responseTimePattern = compile(r"Average response time for all requests: (\d+)")
completedTimePattern = compile(r"Average response time for completed requests: (\d+)")
failedTimePattern = compile(r"Average response time for failed requests: (\d+)")

print """%% Output from prefengine.py running perftest
%% Client host: %s
%% Server host: %s
%% Server port: %s
%% Duration: %s s
%% Column 1: Number of threads
%% Column 2: Number of requests
%% Column 3: Number of completed requests
%% Column 4: Number of failed requests
%% Column 5: Average response time for all requests (ms)
%% Column 6: Average response time for completed requests (ms)
%% Column 7: Average response time for failed requests (ms) """ % (node(), host, port, duration)

for numberOfThreads in threadRange:
    command = "perftest %s %i %i %i" % (host, port, numberOfThreads, duration)
    stdout = popen(command)
    output = stdout.read()
    
    numberOfRequests = numberOfRequestsPattern.search(output).group(1)
    completedRequests = completedRequestsPattern.search(output).group(1)
    failedRequests = failedRequestsPattern.search(output).group(1)
    responseTime = responseTimePattern.search(output).group(1)
    completedTimeMatch = completedTimePattern.search(output)
    if (completedTimeMatch):
        completedTime = completedTimeMatch.group(1)
    else:
        completedTime = "NaN"
    failedTimeMatch = failedTimePattern.search(output)
    if (failedTimeMatch):
        failedTime = failedTimeMatch.group(1)
    else:
        failedTime = "NaN"

    print "%i\t%s\t%s\t%s\t%s\t%s\t%s" % (numberOfThreads,
                                      numberOfRequests,
                                      completedRequests,
                                      failedRequests,
                                      responseTime,
                                      completedTime,
                                      failedTime)

