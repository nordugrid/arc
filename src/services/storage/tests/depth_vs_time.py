#!/usr/bin/env python

import sys,arc
import time

from storage.client import BartenderClient

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

bartender.makeCollection({'0':('/',{})})

false = '0'

try:
    depth = int(sys.argv[1])
    statistics = int(sys.argv[2])
except:
    print "Usage:", sys.argv[0], "depth", "statistics"
    sys.exit(1)

f = open("timings_depth_vs_time_"+str(depth)+"x"+str(statistics)+".dat", 'w')

#################### actual work starts here ###################################

i_path=""
j_path=""

f.write("folder no.\t")
for j in range(statistics):
    f.write("create %d\t"%j)
f.write("AVG\t")
for j in range(statistics):
    f.write("stat %d\t"%j)
f.write("AVG\n")
for i in range(depth):
    i_path+=j_path
    aver_mkdir = 0
    f.write(str(i))
    for j in range(statistics):
        request={}
        j_path = "/folder"+str(i)+str(j)
        request[i_path+j_path] = (i_path+j_path,{})
        start_mkdir = time.time()
    	res = bartender.makeCollection(request)
        time_mkdir = time.time()-start_mkdir
        aver_mkdir += time_mkdir
        f.write("\t%g"%time_mkdir)
        print "mkdir:", i,j,time_mkdir
    f.write("\t%g"%(aver_mkdir/statistics))
    aver_stat = 0
    for j in range(statistics):
        request={}
        j_path = "/folder"+str(i)+str(j)
        start_stat = time.time()
        res= bartender.stat({i_path+j_path:(i_path+j_path)})
        time_stat = time.time()-start_stat
        aver_stat += time_stat
        print "stat:", i,j,time_stat
        f.write("\t%g"%time_stat)
    f.write("\t%g\n"%(aver_stat/statistics))
f.close()

