import arc
from storage.service import Service
from storage.common import transfer_uri, create_response
from storage.common import create_response, get_child_nodes, true
import commands

class Transfer:

        def __init__(self):

                print "Transfer Constructor..."

        def transferData(self, hostnames,options):

                print "Inside transferData function of Transfer .."
		response = {}
		for host in hostnames:
				
			externalURL = 'gsiftp://'+str(host)		
			if not options:
				print 'arcls '+externalURL		
				status, output = commands.getstatusoutput('arcls '+externalURL)	
			else:
				print 'arcls '+str(options[0])+' '+str(externalURL)
				status, output = commands.getstatusoutput('arcls '+str(options[0])+' '+str(externalURL))
			print status
			print output	
			if status == 0 and output.find('ERROR') == -1 or output.find('Failed') == -1:
				  
				response[host] = (status,output)
			
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
                hostnames = []
		options = []
		for index in range(len(request_node)):
			hostnames.append(str(request_node[index].Get('hostname')))
			options.append(str(request_node[0].Get('options')) )
		response = self.transfer.transferData(hostnames,options)
		print response
		return create_response('transfer:transferData',
                               ['transfer:ID','transfer:status','transfer:output'], response, self.newSOAPPayload())
		
