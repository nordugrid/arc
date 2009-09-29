#!/usr/bin/env python

import sys,arc
import time, random

from storage.common import ahash_list_guid
from storage.client import AHashClient

ssl_config = {}
AHashURL = ''
root_logger = arc.Logger_getRootLogger()
root_logger.addDestination(arc.LogStream(sys.stdout))
root_logger.setThreshold(arc.ERROR)

try:
    user_config = arc.UserConfig('')
    key_file = user_config.KeyPath()
    if key_file:
        ssl_config['key_file'] = key_file
    cert_file = user_config.CertificatePath()
    if cert_file:
        ssl_config['cert_file'] = cert_file
    proxy_file = user_config.ProxyPath()
    if proxy_file:
        ssl_config['proxy_file'] = proxy_file
    ca_file = user_config.CACertificatePath()
    if ca_file:
        ssl_config['ca_file'] = ca_file
    ca_dir = user_config.CACertificatesDirectory()
    if ca_dir:
        ssl_config['ca_dir'] = ca_dir
    AHashURL = '' #str(user_config.ConfTree().Get('AHashURL'))
except:
    pass
if not AHashURL:
    try:
        AHashURL = os.environ['ARC_BARTENDER_URL']
        print '- The URL of the AHash:', AHashURL
    except:
        AHashURL = 'http://localhost:60000/AHash'
        print '- ARC_BARTENDER_URL environment variable not found, using', AHashURL
try:
    needed_replicas = int(os.environ['ARC_NEEDED_REPLICAS'])
except:
    needed_replicas = 1
if AHashURL.startswith('https') and not user_config:
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
