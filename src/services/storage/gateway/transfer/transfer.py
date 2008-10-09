"""
Gateway Component of ARC1 storage System. 

Service Class = TransferService
Worker Class  = Transfer

Author: Salman Zubair Toor
email: salman.toor@it.uu.se

"""



import arc
from storage.service import Service
from storage.common import transfer_uri, create_response
from storage.common import create_response, get_child_nodes, true
import commands

from storage.logger import Logger
log = Logger(arc.Logger(arc.Logger_getRootLogger(), 'Storage.TransferService'))

class Transfer:

        def __init__(self):

                print "Transfer Constructor..."

        def transferData(self, request):


		"""This method receives the hostname, flags, protocol, port in the 
                request and currently uses the arc commans to check the status of 
		requested file/directory from the external store. 

                This method returns the requestID, status of the request and output 
                generated from the external store"""

                print "Inside transferData function of Transfer .."
		print request
		response = {}
		for host in request.keys():
			if request[host]['protocol'] == 'gridftp': 	
				
				externalURL = 'gsiftp://'+str(host)+':'+request[host]['port']+'/'+request[host]['path']
				print 'arcls '+request[host]['flags']+' '+externalURL		
				status, output = commands.getstatusoutput('arcls '+request[host]['flags']+' '+externalURL)	
			#else:
			#	print 'arcls '+str(options[0])+' '+str(externalURL)
			#	status, output = commands.getstatusoutput('arcls '+str(options[0])+' '+str(externalURL))
			print status
			print output	
			if status == 0: 
	
				if output.find('ERROR') == -1 or output.find('Failed') == -1:
					response[host] = [status,output]
				else:
					
					response[host]	= [-1, 'Error while processing the request \n'+str(output)]		
			else: 

				response[host]  = [-1, 'Internal error while executing request \n'+output]			
		return response 

class TransferService(Service):

        def __init__(self, cfg):

                print "TransferService Constructor..."
                request_names = ['transferData']
                Service.__init__(self,'Transfer', request_names, 'transfer',transfer_uri)
                self.transfer = Transfer()

        def transferData(self, inpayload):

		"""This method receives the hostname, flags, protocol, port in the 
		inpayload and send this information to the working class of the 
		TransferService class

		This method returns the requestID, status of the request and output 
		generated from the external store"""
	
                print "\n --- \n Inside list function of TransferService"
                print "Message from the client:", inpayload.GetXML()
                print "\n ---"

                request_node = get_child_nodes(inpayload.Child())
             	request = {}
		for index in range(len(request_node)):
			request[str(request_node[index].Get('hostname'))] = {'flags':str(request_node[index].Get('flags')), 'protocol':str(request_node[index].Get('protocol')), 'port':str(request_node[index].Get('port')), 'path':str(request_node[index].Get('path'))}
		response = self.transfer.transferData(request)
		print response
		return create_response('transfer:transferData',
                               ['transfer:host','transfer:status','transfer:output'], response, self.newSOAPPayload())
		
