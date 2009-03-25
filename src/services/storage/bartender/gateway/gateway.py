"""
Gateway Component of ARC1 storage System. 

Service Class = GatewayService
Worker Class  = Gateway

Author: Salman Zubair Toor
email: salman.toor@it.uu.se

"""
import arc
import time
from arcom import get_child_nodes, datapoint_from_url
from arcom.service import gateway_uri, true, create_response
from arcom.service import Service 
import commands
import os
import base64
from arcom.logger import Logger
log = Logger(arc.Logger(arc.Logger_getRootLogger(), 'Bartender.gateway.Gateway'))

class Gateway:

    def __init__(self,cfg):
        #print "Gateway constructor..."
        self.service_name = 'Gateway'
        self.cfg = cfg
        self.proxy_store = str(self.cfg.Get('ProxyStore'))
        self.ca_dir = str(self.cfg.Get('CACertificatesDir'))
        if len(self.proxy_store) == 0:
           log.msg(arc.DEBUG,'proxy store is not accessable.')
         
    def get(self, auth ,sourceURL, flags):
        response = {}  
        status = ''  
        protocol = ''
        proxyfile = base64.b64encode(auth.get_identity()) 
        if len(self.proxy_store) == 0:
           log.msg(arc.DEBUG,'proxy store is not accessable.')
        filepath = self.proxy_store+'/'+proxyfile+'.proxy'
        if os.path.isfile(filepath):
            url = arc.URL(sourceURL);
            handle = arc.DataHandle(url);
            handle.__deref__().AssignCredentials(filepath,'','',self.ca_dir) 
            (files, status) = handle.__deref__().ListFiles(True);
            if files:
                status  = 'successful'
                for file in files:
                    #print file.GetName()
                    if sourceURL[:6] == 'gsiftp':
                        protocol = 'gridftp'
                    elif sourceURL[:3] == 'srm':
                        protocol = 'srm'
                    response[file.GetName()] = {'turl':sourceURL,'status': status,'protocol':protocol}
            else:
                status = 'failed: cannot access file'
                response[url] = {'turl':'','status': status,'protocol':''}
        else:
            status = 'cannot find valid credentials'
            response[url] = {'turl':'','status': status,'protocol':''}
        log.msg(arc.DEBUG,'get response: %s',response)
        return response
            
    def put(self, auth, url, flags):
        response = {}
        if url[:6] == 'gsiftp':
            protocol = 'gridftp'
            status = 'done'
        elif url[:3] == 'srm':
            protocol = 'srm'
            status = 'done'
        else:
            protocol = 'unkonwn'
            status = 'failed' 
        response[url] = {'turl':url,'status':status, 'protocol':protocol}
        return response

    def list(self, auth, url, flags=''):
        """Input arguments
        URL of the file or directory """
        response = {}
        tmpList = []
        longlisting = '-l' in flags
        status = ''
        if url.startswith('gsiftp'):
            protocol = 'gridftp'
        elif url.startswith('srm'):
            protocol = 'srm'
        else:
            protocol = 'unkonwn'
        if len(self.proxy_store) == 0:
            log.msg(arc.DEBUG,'proxy store is not accessable.')
        if protocol != 'unknown':
            proxyfile = base64.b64encode(auth.get_identity())
            filepath = self.proxy_store + '/' + proxyfile + '.proxy'
            if os.path.isfile(filepath):
                dp = datapoint_from_url(url)
                dp.AssignCredentials(filepath, '', '', self.ca_dir)
                (files, stat) = dp.ListFiles(longlisting)
                if files:
                    status = 'found'
                    for f in files:
                        if longlisting:
                            #print f.GetName(), " ", f.GetSize(), " ", f.GetCreated()
                            if (f.GetType() == 1):
                                type = 'file'
                            elif (f.GetType() == 2):
                                type = 'dir'
                            else:
                                type = 'known'       
                            tmpList.append(f.GetName() + ':' + str(f.GetSize()) + ':' + type + '\n')
                        else:
                            tmpList.append(f.GetName())
                else:
                    status = 'Cannot access external store. Reason: %s' % str(stat)
            else:
                status = 'Your proxy cannot be found. Please delegate your credentials!'
        response[url] = {'list': tmpList, 'status': status, 'protocol': protocol}
        log.msg(arc.DEBUG, 'list response: %s', response)
        return response
        
    def remove(self, auth, url, flags):
        """ remove file or direcotory """
        response = {}
        protocol = ''
        status = ''
        if url[:6] == 'gsiftp':
            protocol = 'gridftp'
        elif url[:3] == 'srm':
            protocol = 'srm'
        else:
            protocol = 'unkonwn'
        if len(self.proxy_store) == 0:
            status = 'failed' 
            log.msg(arc.DEBUG,'proxy store is not accessable.')
        if protocol != 'unknown':
            proxyfile = base64.b64encode(auth.get_identity())
            filepath = self.proxy_store+'/'+proxyfile+'.proxy'
            if os.path.isfile(filepath):
                externalURL = arc.URL(url)
                handle = arc.DataHandle(externalURL)
                handle.__deref__().AssignCredentials(filepath,'','',self.ca_dir)
                status = handle.__deref__().Remove()
                #status = 'successful'
        else:
            status = 'failed'  
        response[url]={'status':str(status),'protocol':protocol}
        #print "File or directory removed"     
        return response 
