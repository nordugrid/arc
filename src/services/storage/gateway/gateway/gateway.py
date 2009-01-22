"""
Gateway Component of ARC1 storage System. 

Service Class = GatewayService
Worker Class  = Gateway

Author: Salman Zubair Toor
email: salman.toor@it.uu.se

"""
import arc
import time
from storage.common import create_response, gateway_uri, get_child_nodes, true
from storage.service import Service 
import commands

from storage.logger import Logger
log = Logger(arc.Logger(arc.Logger_getRootLogger(), 'Storage.Gateway'))

class Gateway:

    def __init__(self,cfg):
        #print "Gateway constructor..."
   	self.service_name = 'Gateway' 
    def get(self, sourceURL, flags):
        response ={}    
	url = arc.URL(sourceURL);
        handle = arc.DataHandle(url);
        usercfg = arc.UserConfig("");
        cred = arc.Config(arc.NS());
        usercfg.ApplySecurity(cred);
        handle.__deref__().AssignCredentials(cred);
        (files, status) = handle.__deref__().ListFiles(True);
        for file in files:
        	#print file.GetName()
		response[file.GetName()] = [sourceURL,status] 
        return response
            
    def put(self, request, flags):
	response = {}
        #print request
        return response

    def list(self, request, flags):
        """Input arguments
        URL of the file or directory """
        response = {}
	tmpList = []
        url = arc.URL(request)
	handle = arc.DataHandle(url)
	usercfg = arc.UserConfig("")
	cred = arc.Config(arc.NS())
	usercfg.ApplySecurity(cred)
	handle.__deref__().AssignCredentials(cred)
	(files, status) = handle.__deref__().ListFiles(True)
	print status	 
	if (flags == '-l'):
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

	response[request]=[status,''.join(tmpList)]
	return response
    
    def remove(self, sourceURL, flags):
	
        """ remove file or direcotory """
	response = {}
        #url = arc.URL(sourceURL)
	#handle = arc.DataHandle(url)
        #usercfg = arc.UserConfig("")
        #cred = arc.Config(arc.NS())
        #usercfg.ApplySecurity(cred)
        #handle.__deref__().AssignCredentials(cred)
        #(files, status) = handle.__deref__().ListFiles(True)
	
	#print "File or directory removed"     
	return response	
	

""" A high level service that contacts the externalStorageInformationService and TransferService To 
    to provide a bridge between ARC Storage Manager and thrid party storage system."""

class GatewayService(Service):
    
    def __init__(self, cfg):

        #print "GatewayService Constructor..."

	self.service_name = 'Gateway' 
        request_names = ['get','list','put','remove']
        Service.__init__(self, 'Gateway', request_names, 'gateway', gateway_uri, cfg)
        self.gateway = Gateway(cfg)
                
    def get(self, inpayload):
    
        #print "Inside getFile function of GatwayService"
        #print "Message from the client:", inpayload.GetXML()    
        request_node = get_child_nodes(inpayload.Child())
        sourceURL = str(request_node[0].Get('sourceURL'))
        flags = str(request_node[0].Get('flags'))
        response = self.gateway.get(sourceURL,flags)
        
        #print response

        return create_response('gateway:getFile', 
            ['gateway:file', 'gateway:url', 'gateway:status'], response, self.newSOAPPayload() )
    
    def put(self, inpayload):

        #print "Inside getFile function of GatwayService"
        #print "Message from the client:", inpayload.GetXML()
        request_node = get_child_nodes(inpayload.Child())
        sourceURL = str(request_node[0].Get('sourceURL'))
        #print '\n sourceURL = ', sourceURL
        flags = str(request_node[0].Get('flags'))

        response = self.gateway.put(soruceURL,flags)
        #print response
        
        return create_response('gateway:putFile',
                        ['gateway:file', 'gateway:url', 'gateway:status'], response, self.newSOAPPayload() )
    
    def list(self, inpayload):
        """ This method recieves the message from the client and forword 
        this message to the working class of the service class. 
        This method returns the requestID, status of the request and the 
        output genarated from the external store"""
        #print "\n --- \n Inside list function of GatewayService"
        #print "Message from the client:", inpayload.GetXML()
        #print "\n ---"
        request_node = get_child_nodes(inpayload.Child())
        externalrequest = str(request_node[0].Get('externalURL'))                
        flags = str(request_node[0].Get('flags'))
        response = self.gateway.list(externalrequest, flags)
	#print response
        return create_response('gateway:list',
                        ['gateway:url','gateway:status', 'gateway:info'], response, self.newSOAPPayload() )
    
    def remove(self, inpayload):
        """ remove the file or directory """
        #print "Message from the client:", inpayload.GetXML()
        request_node = get_child_nodes(inpayload.Child())
        sourceURL = str(request_node[0].Get('sourceURL'))
        print '\n sourceURL = ', sourceURL
        flags = str(request_node[0].Get('flags'))
	response = self.gateway.remove(sourceURL, flags)
	#print response 	
