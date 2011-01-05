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
        self.ssl_config={}
        if len(str(self.cfg.Get('ProxyStore'))) != 0:
            self.ssl_config['proxy_store']=str(self.cfg.Get('ProxyStore'))
        else:
            log.msg(arc.VERBOSE,'proxy store is not accessable.')
        if len(str(self.cfg.Get('CACertificatesDir'))) != 0:
            self.ssl_config['ca_dir']=str(self.cfg.Get('CACertificatesDir'))
        else:
            log.msg(arc.VERBOSE,'Certificate directory is not accessable! Check configuration file.')


    def get(self, auth ,sourceURL, flags):
        response = {}  
        status = ''  
        protocol = ''
        proxyfile = base64.b64encode(auth.get_identity()) 
        if self.ssl_config.has_key('proxy_store'):
            filepath = self.ssl_config["proxy_store"]+'/'+proxyfile+'.proxy'
            if os.path.isfile(filepath):
                dp = datapoint_from_url(sourceURL,self.ssl_config)
                (files,stat) = dp.List()
                if files:
                    status  = 'successful'
                    for file in files:
                        #print file.GetName()
                        if sourceURL.startswith('gsiftp'):
                            protocol = 'gridftp'
                        elif sourceURL.startswith('srm'):
                            protocol = 'srm'
                        elif sourceURL.startswith('arc'):
                            protocol = 'arc'

                        response[file.GetName()] = {'turl':sourceURL,'status': status,'protocol':protocol}
                else:
                    status = 'Failed: cannot access file'
                    response[sourceURL] = {'turl':'','status': status,'protocol':''}

            else:
                status = 'Cannot find valid credentials'
                response[sourceURL] = {'turl':'','status': status,'protocol':''}
                log.msg(arc.VERBOSE,'Get response: %s',response)

        else:
            log.msg(arc.VERBOSE,'Proxy store is not accessable.')
            status = 'Proxy store is not accessable.'
            response[sourceURL] = {'turl':'','status': status,'protocol':''}


        return response
            
    def put(self, auth, url, flags):
        response = {}
        if url.startswith('gsiftp'):
            protocol = 'gridftp'
            status = 'done'
        elif url.startswith('srm'):
            protocol = 'srm'
            status = 'done'
        elif url.startswith('arc'):
            protocol = 'arc'
            status = 'done'
        else:
            protocol = 'unkonwn'
            status = 'Failed' 
        response[url] = {'turl':url,'status':status, 'protocol':protocol}
        return response

    def list(self, auth, url, flags=''):
        """Input arguments
        URL of the file or directory """
        response = {}
        tmpList = []
        #longlisting = '-l' in flags
        longlisting = True #'-l' in flags
        status = ''
        if url.startswith('gsiftp'):
            protocol = 'gridftp'
        elif url.startswith('srm'):
            protocol = 'srm'
        elif url.startswith('arc'):
            protocol = 'arc'
            status = 'done' 
        else:
            protocol = 'unkonwn'
        if self.ssl_config.has_key('proxy_store'): 
            if protocol != 'unknown':
                proxyfile = base64.b64encode(auth.get_identity())
                filepath = self.ssl_config['proxy_store'] + '/' + proxyfile + '.proxy'
                if os.path.isfile(filepath):

                    self.ssl_config['proxy_file']=filepath
                    #ssl_config['ca_dir']=self.ca_dir
                    #print "self.ssl_config: "
                    #print  ssl_config
                    #print "\n URL: "+url
                    dp = datapoint_from_url(url,self.ssl_config)
                    (files,stat) = dp.List()
                    if files:
                        status = 'found'
                        for f in files:
                            if longlisting:
                                #print f.GetName(), ' ', f.GetSize(), ' ', f.GetCreated(), ' ',f.GetType()
                                if (f.GetType() == 1):
                                    type = 'file'
                                elif (f.GetType() == 2):
                                    type = 'dir'
                                else:
                                    type = '---'       
                                tmpList.append(f.GetName() + ' : ' + str(f.GetSize()) + ' : ' + type)

                            else:
                                tmpList.append(f.GetName())
                    else:
                        status = 'Cannot access external store. Reason: %s' % str(stat)

                else:
                    status = 'Proxy cannot be found. Please delegate your credentials!'
            response[url] = {'list': tmpList, 'status': status, 'protocol': protocol}
            log.msg(arc.VERBOSE, 'List response: %s', response)
        else:
            log.msg(arc.VERBOSE,'Proxy store is not accessable.')
            status = 'Failed' 
            response[url] = {'list': tmpList, 'status': status, 'protocol': protocol} 

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
        if not self.ssl_config.has_key('proxy_store'):
            status = 'Failed to access proxy store' 
            log.msg(arc.VERBOSE,'Proxy store is not accessable.')
        if protocol != 'unknown':
            proxyfile = base64.b64encode(auth.get_identity())
            filepath = self.ssl_config['proxy_store']+'/'+proxyfile+'.proxy'
            if os.path.isfile(filepath):
                dp = datapoint_from_url(url,self.ssl_config)
                status = dp.Remove()
                if status == 0:
                    respstatus = 'deleted'
                else:
                    respstatus = 'Failed to delete file, external status: '+str(status)

        else:
            status = 'Failed unknown protocol.'  
        response[url]={'status':str(status),'protocol':protocol}
        #print "File or directory removed"     
        return response 
