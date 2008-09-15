import arc
from storage.service import Service
from storage.common import transfer_uri, create_response
from storage.common import create_response, get_child_nodes, true
import commands

class Transfer:

        def __init__(self):

                print "Transfer Constructor..."

        def transferData(self, request,options):

                print "Inside transferData function of Transfer .."
		print request
		
		externalURL = 'gsiftp://'+str(request)		
		
		print "external URL "+externalURL
		if options is None:		
			status, output = commands.getstatusoutput('arcls '+externalURL)	
		else:
			status, output = commands.getstatusoutput('arcls '+options+' '+externalURL)
		#print status
		#print output
		response = {}
		response['1'] = (status,output)
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
                request = str(request_node[0].Get('hostname'))
		options = str(request_node[0].Get('options'))
		#print request 

		response = self.transfer.transferData(request,options)
		
		return create_response('transfer:transferData',
                               ['transfer:ID','transfer:status','transfer:output'], response, self.newSOAPPayload())
		
