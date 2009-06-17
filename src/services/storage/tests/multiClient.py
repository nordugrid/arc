#!/usr/bin/env python

import time
import threading
from threading import Thread
import os, sys, arc, time
from storage.client import BartenderClient
import random
import socket

""" HOW TO RUN
python multiclient.py <start Collection> <stop Collection> <number of Clients> <subCollection Name> <client Directory number>
 The script required a directory structure, starting from the current working directory. 
Example:
 
 multiClient/client-1/
 multiClient/client-2/
 multiClient/client-3/
  ....
  ....
 multiClient/client-10/
 
So this way each multiple client will have a separate directory and all the clients will create an individual file in that directory. 
The last argument in the scrip pointing towards the directory's last number. I mean in the case of  multiClient/client-3/ the last argument points to 3.    

Run this program and then copy all the files into one directory and run the parsr.py script. 
"""
def myfunc(thread, start, stop, subcoll, clientnum):

    f = open('./multiClient/client-'+clientnum+'/client-'+str(thread)+'_'+str(start)+'-'+str(stop)+'.dat'+socket.gethostname(), 'w')
    request={}
    i_path = '/'+subcoll+str(thread)
    request[i_path] = (i_path,{})
    while True:
        try:		
            res = bartender.makeCollection(request)
            print 'successfully Created'
            break
            
        except:       
            print 'Faile: try retry ... '

    #res = bartender.stat(request)
    #time.sleep(random.random())
    print res
    for i in range(start,stop):
        request={}
        i_path = '/'+subcoll+str(thread)+"/w"+str(i)
        request[i_path] = (i_path,{})
        start_create = time.time()
        while True:
	    try:
                res = bartender.makeCollection(request)
                print 'successfully Created:'
                break
            except:
                print 'Failed: retry .... '
        print res 
        stop_create = time.time()
        f.write(str(i)+"\t"+str(stop_create-start_create)+"\n") 
        #res = bartender.stat(request)
        #while res[i_path] != 'done':
	#	time.sleep(random.random()*1.5)
	#	print 'retry ...'
        #        res = bartender.makeCollection(request) 
        #res = bartender.stat({'0':('/c'+str(thread)+"/w"+str(i))})
        #stop_create = time.time()
	#f.write(str(i)+"\t"+str(stop_create-start_create)+"\n")
        #time.sleep(random.random()*0.1)
    f.close()

ssl_config = {}
BartenderURL = ''

try:
    user_config = arc.UserConfig('')
    config_xml = user_config.ConfTree()
    key_file = str(config_xml.Get('KeyPath'))
    if key_file:
        ssl_config['key_file'] = key_file
    cert_file = str(config_xml.Get('CertificatePath'))
    if cert_file:
        ssl_config['cert_file'] = cert_file
    proxy_file = str(config_xml.Get('ProxyPath'))
    if proxy_file:
        ssl_config['proxy_file'] = proxy_file
    ca_file = str(config_xml.Get('CACertificatePath'))
    if ca_file:
        ssl_config['ca_file'] = ca_file
    ca_dir = str(config_xml.Get('CACertificatesDir'))
    if ca_dir:
        ssl_config['ca_dir'] = ca_dir
    BartenderURL = str(config_xml.Get('BartenderURL'))
except:
    pass
if not BartenderURL:
    try:
        BartenderURL = os.environ['ARC_BARTENDER_URL']
        print '- The URL of the Bartender:', BartenderURL
    except:
        BartenderURL = 'http://localhost:60000/Bartender'
        print '- ARC_BARTENDER_URL environment variable not found, using', BartenderURL
try:
    needed_replicas = int(os.environ['ARC_NEEDED_REPLICAS'])
except:
    needed_replicas = 1
if BartenderURL.startswith('https') and not user_config:
    try:
        ssl_config['key_file'] = os.environ['ARC_KEY_FILE']
        ssl_config['cert_file'] = os.environ['ARC_CERT_FILE']
        print '- The key file:', ssl_config['key_file']
        print '- The cert file:', ssl_config['cert_file']
        if os.environ.has_key('ARC_CA_FILE'):
            ssl_config['ca_file'] = os.environ['ARC_CA_FILE']
            print '- The ca file:', ssl_config['ca_file']
        else:
            ssl_config['ca_dir'] = os.environ['ARC_CA_DIR']
            print '- The ca dir:', ssl_config['ca_dir']
    except:
        ssl_config = {}
        raise RuntimeError, '- ARC_KEY_FILE, ARC_CERT_FILE , ARC_CA_FILE or ARC_CA_DIR environment variable not found, SSL disabled'


bartender = BartenderClient(BartenderURL, False, ssl_config)


false = '0'

start = int(sys.argv[1])
stop = int(sys.argv[2])
numthread = int(sys.argv[3])
subcoll = sys.argv[4]
clientnum = sys.argv[5]
i_path=""
j_path=""
threadpool = []
for i in range(numthread):
    t = Thread(target=myfunc, args=(i,start,stop, subcoll, clientnum))
    t.start()
    threadpool.append(t)
while threading.activeCount()> 1 :
    print "Threads are alive"
    time.sleep(5)

#sum=[]

#list = os.listdir('multiClient/client-'+clientnum)
#for filename in list:
#    file = open('multiClient/client-'+clientnum+'/'+filename)
#    ind = 0
#    tmp = 0.0
#    while 1:
#        line = file.readline()
#        if not line:
#            break
    #if '- done ' in line:
#        print line[2:10]
#        tmp = tmp+float(line[2:10])
#        ind = ind + 1
#    file.close()
#    sum.append(tmp)
#    print 'total values', ind 
#ss = 0
#for val in sum:
#    ss = ss + val
#print sum

#print 'No. of clients :\t Mim-time :\t Max-Time :\t Avg-Time :\t PerCall-Time  \n' 
#print  '\t', numthread, ':\t\t', min(sum), ':\t', max(sum), ':\t', ss/len(sum), ':\t',max(sum)/(numthread*(stop-start))

#exist  = os.path.exists('multiClient/multiClient.txt')
#if exit == 'True':
#    f = open('multiClient/multiClient.txt', 'a')     
#    f.write('\t', numthread, ':\t\t', min(sum), ':\t', max(sum), ':\t', ss/len(sum), ':\t',max(sum)/(numthread*(stop-start)))
#    f.close()
#    print 'results are appended to the file: multiClient/multiClient.txt'
#else:
#    f = open('multiClient/multiClient.txt', 'w')
#    f.write('No. of clients :\t Mim-time :\t Max-Time :\t Avg-Time :\t PerCall-Time  \n')
#    f.write('\t', numthread, ':\t\t', min(sum), ':\t', max(sum), ':\t', ss/len(sum), ':\t',max(sum)/(numthread*(stop-start))) 	
#    f.close()
#    print 'results are written to the file: multiClient/multiClient.txt'

