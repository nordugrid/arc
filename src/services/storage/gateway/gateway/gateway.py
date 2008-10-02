"""
Gateway Component of ARC1 storage System. 

Service Class = GatewayService
Worker Class  = Gateway

Author: Salman Zubair Toor
email: salman.toor@it.uu.se

"""
import arc
import time
from storage.common import create_response, gateway_uri, externalInfo_uri, transfer_uri,get_child_nodes, true
from storage.client import ExternalStorageInformationClient, TransferClient
from storage.service import Service 
import commands
class Gateway:

	def __init__(self, externalInfo_url, transfer_url):
		
		print "Gateway constructor..."
		self.externalstorageinformation = ExternalStorageInformationClient(externalInfo_url)
                self.transfer = TransferClient(transfer_url) 
	
	def getFile(self, request, flags):
	

		transfer_request = {}
		response ={}	
		if 'dCache' in request[0]:

                        """Get the information about the external stores"""
                        info_response = self.externalstorageinformation.getInfo('dCache')
			print info_response
			if info_response.keys()[0] != 'NoHostFound':
				for res in info_response.keys():

					transfer_request[res] = {'flags':flags,'protocol':info_response[res]['protocol'],'port':info_response[res]['port'],'path':str(request[0].split('dCache')[1])}
				print "request from getFile of gateway for transfer"
				print transfer_request
				"""Sends the request to the Transfer service to check the 
				availablity of the file/directory in the external store."""
				response = self.transfer.transferData(transfer_request)
				for host in response.keys():
					response[host].append(info_response[host]['protocol'])
					response[host].append(info_response[host]['port'])
					response[host].append( str(request[0].split('dCache')[1]) )
		else:	
			response['1']=['-2','NoHostFound']
                print "\n"
		print response

		for host in response.keys():
			if response[host][0] == '-1':
				del response[host]

		return response
			
	def putFile(self, request, flags):

		transfer_request = {}
		response = {}
		print request
		if 'dCache' in request[1]:
                        """Get the information about the external stores"""
                        response = self.externalstorageinformation.getInfo('dCache')
                	print response
                else:
                        response['NoHostFound']=['','']
                print "\n"
                print response
                return response

	def list(self, request, flags):
		
		"""Input arguments
		request: path of the file/directory in the external store togather 
		with the type of the store. For example
		
		/mycollection/dCache/pnfs/uppmax.uu.se/data
		  
		this path have subcollection dCache that shows the target external 
		store is DCACHE. Remaining part of the URL is path of the file/directory
		inside the external store 

		This function contact to the externalStorageInformation service and get 
		all the possible information about the external store of requested type. 
		Next step is to cotact to the available external stores to find out the 
		availability of the requested file/directory  

		This function will return the requestID, status of the request and the 
		output of the request from external store.  """
		
		transfer_request = {}
		
		if 'dCache' in request:
			
			"""Get the information about the external stores""" 
			info_response = self.externalstorageinformation.getInfo('dCache')
                	
			print info_response
			if info_response.keys()[0] != 'NoHostFound':
				for res in info_response.keys():
					
					transfer_request[res] = {'flags':flags,'protocol':info_response[res]['protocol'],'port':info_response[res]['port'],'path':str(request.split('dCache')[1])}
				print "request from gateway for transfer"
				print transfer_request
				
				"""Sends the request to the Transfer service to check the 
				availablity of the file/directory in the external store."""		
				response = self.transfer.transferData(transfer_request)
                	else:

				response['1']=['-2','NoHostFound']
		print "\n "	
		print response

		for host in response.keys():
			if response[host][0] == '-1':	
				del response[host]

		return response
	
from storage.logger import Logger

log = Logger(arc.Logger(arc.Logger_getRootLogger(), 'Gateway'))

""" A high level service that contacts the externalStorageInformationService and TransferService To 
    to provide a bridge between ARC Storage Manager and thrid party storage system."""
class GatewayService(Service):
	

	def __init__(self, cfg):

		print "GatewayService Constructor..."
  
		request_names = ['getFile','list','putFile']
		self.log = log
		Service.__init__(self, 'Gateway', request_names, 'gateway', gateway_uri, self.log)
		externalInfo_url = str(cfg.Get('ExternalStorageInformationURL'))
		transfer_url = str(cfg.Get('TransferURL'))
		
		self.externalstorageinformation = ExternalStorageInformationClient(externalInfo_url)
		self.tansfer = TransferClient(transfer_url)
		self.gateway = Gateway(externalInfo_url, transfer_url)
				
	def getFile(self, inpayload):
	
		print "Inside getFile function of GatwayService"
		print "Message from the client:", inpayload.GetXML()	
		request_node = get_child_nodes(inpayload.Child())
		sourceURL = str(request_node[0].Get('sourceURL'))
		print '\n sourceURL = ', sourceURL 
		destinationURL =  str(request_node[0].Get('destinationURL'))	
		print '\n destinationURL = ', destinationURL
		flags = str(request_node[0].Get('flags'))
		
		request = [sourceURL, destinationURL]
		response = self.gateway.getFile(request,flags)
		
		print response

		return create_response('gateway:getFile', 
			['gateway:host', 'gateway:status', 'gateway:output','gateway:protocol', 'gateway:port','gateway:path'], response, self.newSOAPPayload() )
 	
	def putFile(self, inpayload):

		print "Inside getFile function of GatwayService"
                print "Message from the client:", inpayload.GetXML()
                request_node = get_child_nodes(inpayload.Child())
                sourceURL = str(request_node[0].Get('sourceURL'))
                print '\n sourceURL = ', sourceURL
                destinationURL =  str(request_node[0].Get('destinationURL'))
                print '\n destinationURL = ', destinationURL
                flags = str(request_node[0].Get('flags'))

                request = [sourceURL, destinationURL]
                response = self.gateway.putFile(request,flags)
                
                for host in response.keys():
			response[host] = [response[host]['protocol'],response[host]['port']]
		print response
		
		return create_response('gateway:putFile',
                        ['gateway:host', 'gateway:protocol', 'gateway:port'], response, self.newSOAPPayload() )
	
	def list(self, inpayload):

		""" This method recieves the message from the client and forword 
		this message to the working class of the service class. 
		This method returns the requestID, status of the request and the 
		output genarated from the external store"""


		print "\n --- \n Inside list function of GatewayService"
		print "Message from the client:", inpayload.GetXML()
                print "\n ---"

		request_node = get_child_nodes(inpayload.Child())
                externalrequest = str(request_node[0].Get('externalURL'))                
		flags = str(request_node[0].Get('flags'))
		response = self.gateway.list(externalrequest, flags)
		 

		return create_response('gateway:list',
                        ['gateway:requestID', 'gateway:status', 'output'], response, self.newSOAPPayload() )

