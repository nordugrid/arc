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
	
	def getFile(self, requests):
		
		xx = self.externalstorageinformation.getInfo(requests)
		print xx
		response = {}
		print "Inside getFile function of Gateway.."
		xx = self.transfer.transferData()
		print xx
		
		response['1111'] = ('OK', 'TURL')
                return response
			
	def putFile(self, requests):

                xx = self.externalstorageinformation.getInfo(requests)
                print xx
                response = {}
                print "Inside putFile function of Gateway.."
                xx = self.transfer.transferData()
                print xx
		response['2222'] = ('OK','Done' )
                return response

	def list(self, requests, flags):

		request = {}
		
		if 'dCache' in requests:
			response = self.externalstorageinformation.getInfo('dCache')
                	print response
			if response.keys()[0] != 'NoHostFound':
				for res in response.keys():
					
					request[res] = (flags,response[res][0],response[res][1],str(requests.split('dCache')[1]))
				print "request from gateway for transfer"
				print request
					
				response = self.transfer.transferData(request)
                	else:

				response['1']=('-2','NoHostFound')
		print "\n"
		print response

		for host in response.keys():
			if response[host][0] == '-1':	
				del response[host]

		return response
	

""" A high level service that contacts the externalStorageInformationService and TransferService To 
    to provide a bridge between ARC Storage Manager and thrid party storage system."""
class GatewayService(Service):
	
	def __init__(self, cfg):

        	print "GatewayService Constructor..."
  
		request_names = ['getFile','list','putFile']
		Service.__init__(self, 'Gateway', request_names, 'gateway', gateway_uri)
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
		
		requests = {'sourceURL':sourceURL, 'destinationURL':destinationURL}
		response = self.gateway.getFile(requests)
		return create_response('gateway:getFile', 
			['gateway:requestID', 'gateway:success', 'gateway:TURL'], response, self.newSOAPPayload() )
 	
	def putFile(self, inpayload):

                print "Inside putFile function of GatwayService"
                print "Message from the client:", inpayload.GetXML()
                request_node = get_child_nodes(inpayload.Child())
                sourceURL = str(request_node[0].Get('sourceURL'))
                print '\n sourceURL = ', sourceURL
                destinationURL =  str(request_node[0].Get('destinationURL'))
                print '\n destinationURL = ', destinationURL

                requests = {'sourceURL':sourceURL, 'destinationURL':destinationURL}
                response = self.gateway.putFile(requests)
                return create_response('gateway:putFile',
                        ['gateway:requestID', 'gateway:success', 'gateway:status'], response, self.newSOAPPayload() )

	def list(self, inpayload):

		print "\n --- \n Inside list function of GatewayService"
		print "Message from the client:", inpayload.GetXML()
                print "\n ---"

		request_node = get_child_nodes(inpayload.Child())
                externalrequest = str(request_node[0].Get('externalURL'))                
		flags = str(request_node[0].Get('flags'))
		response = self.gateway.list(externalrequest, flags)
		 

		return create_response('gateway:list',
                        ['gateway:requestID', 'gateway:status', 'output'], response, self.newSOAPPayload() )

