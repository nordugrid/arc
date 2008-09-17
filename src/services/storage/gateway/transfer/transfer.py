import arc
from storage.service import Service
from storage.common import transfer_uri, create_response
from storage.common import create_response, get_child_nodes, true
import commands

class Transfer:

        def __init__(self):

                print "Transfer Constructor..."

        def transferData(self, request):

                print "Inside transferData function of Transfer .."
		print request
		response = {}
		for host in request.keys():
			if request[host][1] == 'gridftp': 	
				
				externalURL = 'gsiftp://'+str(host)+':'+request[host][2]+'/'+request[host][3]
				print 'arcls '+request[host][0]+' '+externalURL		
				status, output = commands.getstatusoutput('arcls '+request[host][0]+' '+externalURL)	
			#else:
			#	print 'arcls '+str(options[0])+' '+str(externalURL)
			#	status, output = commands.getstatusoutput('arcls '+str(options[0])+' '+str(externalURL))
			print status
			print output	
			if status == 0: 
	
				if output.find('ERROR') == -1 or output.find('Failed') == -1:
					response[host] = (status,output)
				else:
					
					response[host]	= (-1, 'Error while processing the request \n'+str(output))		
			else: 
                                response[host]  = (-1, 'Internal error while executing request \n'+output)			
		return response 

class TransferService(Service):

        def __init__(self, cfg):

                print "TransferService Constructor..."
                request_names = ['transferData']
                Service.__init__(self,'Transfer', request_names, 'transfer',transfer_uri)
                self.transfer = Transfer()

        def transferData(self, inpayload):

                print "\n --- \n Inside list function of TransferService"
                print "Message from the client:", inpayload.GetXML()
                print "\n ---"

                request_node = get_child_nodes(inpayload.Child())
             	request = {}
		for index in range(len(request_node)):
			request[str(request_node[index].Get('hostname'))] = (str(request_node[index].Get('flags')), str(request_node[index].Get('protocol')),str(request_node[index].Get('port')), str(request_node[index].Get('path')))
		response = self.transfer.transferData(request)
		print response
		return create_response('transfer:transferData',
                               ['transfer:ID','transfer:status','transfer:output'], response, self.newSOAPPayload())
		
