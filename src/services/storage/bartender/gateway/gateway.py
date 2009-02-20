"""
Gateway Component of ARC1 storage System. 

Service Class = GatewayService
Worker Class  = Gateway

Author: Salman Zubair Toor
email: salman.toor@it.uu.se

"""
import arc
import time
from arcom import get_child_nodes
from arcom.service import gateway_uri, true, create_response
from arcom.service import Service 
import commands
import os
import base64
from arcom.logger import Logger
log = Logger(arc.Logger(arc.Logger_getRootLogger(), 'Storage.Gateway'))

class Gateway:

    def __init__(self,cfg):
        #print "Gateway constructor..."
        self.service_name = 'Gateway'
	self.cfg = cfg
 	self.proxy_store = str(self.cfg.Get('ProxyStore'))
        self.ca_dir = str(self.cfg.Get('CACertificatesDir'))
    def get(self, auth ,sourceURL, flags):
        response = {}  
        status = ''  
        protocol = ''
        proxyfile = base64.b64encode(auth.get_identity()) 
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
        print response
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

    def list(self,auth,url, flags=''):
        """Input arguments
        URL of the file or directory """
        response = {}
        tmpList = []
        status = ''
        protocol = ''
        if url[:6] == 'gsiftp':
            protocol = 'gridftp'
        elif url[:3] == 'srm':
            protocol = 'srm'
        else:
            protocol = 'unkonwn'
        if protocol != 'unknown':

            proxyfile = base64.b64encode(auth.get_identity())
            filepath = self.proxy_store+'/'+proxyfile+'.proxy'
            if os.path.isfile(filepath):
            
                externalURL = arc.URL(url)
                handle = arc.DataHandle(externalURL)
                #usercfg = arc.UserConfig("")
                #cred = arc.Config(arc.NS())
                #usercfg.ApplySecurity()
                handle.__deref__().AssignCredentials(filepath,'','',self.ca_dir)
                (files, stat) = handle.__deref__().ListFiles(True)
                if files:
                    if (flags == '-l'):
                        status = 'successful'
                        for file in files:
                            print file.GetName(), " ", file.GetSize(), " ", file.GetCreated()
                            if ( file.GetType() == 1):
                                type = 'file'
                            elif (file.GetType() == 2):
                                type = 'dir'
                            else:
                                type = 'known'       
                            if (flags == '-l'):      
                                tmpList.append(file.GetName()+':'+str(file.GetSize())+':'+type+'\n')
                            else:
                                tmpList.append(file.GetName()+'\n')
                    else:
                        for file in files:
                            tmpList.append(file.GetName()+',')
                        if len(tmpList) == 0:
                            status = 'no file/directory found'
                        else: 
                            status = 'successful'    
                else:
                    status = 'cannot access file or directory. Check the LN or the credentials. '
                                   
            else:
                status = 'failed: credentials not found'
        
	response[url]={'list':''.join(tmpList),'status':status,'protocol':protocol}

        return response
    def remove(self, auth, url, flags):
        
        """ remove file or direcotory """
        response[url] = {'url':'','status':'','protocol': ''}
        #url = arc.URL(sourceURL)
        #handle = arc.DataHandle(url)
        #usercfg = arc.UserConfig("")
        #cred = arc.Config(arc.NS())
        #usercfg.ApplySecurity(cred)
        #handle.__deref__().AssignCredentials(cred)
        #(files, status) = handle.__deref__().ListFiles(True)
        
        #print "File or directory removed"     
        return response 
        
