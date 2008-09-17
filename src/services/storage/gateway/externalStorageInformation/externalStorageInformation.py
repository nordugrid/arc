import arc
from storage.common import externalInfo_uri, create_response
from storage.common import create_response, get_child_nodes, true
from storage.service import Service
import os 
class ExternalStorageInformation:

	def __init__(self):
	
		print "ExternalStorageInformation Constructor..." 
		
	def getInfo(self, request):
		
		information = {}
		print "Inside getInfo function of ExternalStorageInformation .."	
		if (os.path.isfile("../externalStorageInformation/externalStores")):	
			f = open('../externalStorageInformation/externalStores', "r")
			for line in f.readlines():
				if request == 'dCache':
					if line.split('=')[0] == 'dcache':  
						secondline = line.split('=')[1]
						hostprotocolport = secondline.split(';')
						print hostprotocolport
						if len(hostprotocolport) == 3:
							information[hostprotocolport[1]]= (hostprotocolport[0],hostprotocolport[2].rstrip('\n'))
						elif len(hostprotocolport) == 2:
							information[hostprotocolport[1].rstrip('\n')]= (hostprotocolport[0].rstrip('\n'),'2811')
						else:
							information["NoHostFound"]=('','','')

	
		print information
		if information == {}:
			information["NoHostFound"]=('','','')
		return information					
class ExternalStorageInformationService(Service):

	def __init__(self, cfg):
	
		print "ExternalStorageInformationService Constructor..."
		request_names = ['getInfo']
		Service.__init__(self,'ExternalStorageInformation', request_names, 'externalstorageinformation',externalInfo_uri)
		self.externalInfo = ExternalStorageInformation()
		
	def getInfo(self, inpayload):
		
		print "\n --- \n Inside list function of ExternalStorageInformationService"
                print "Message from the client:", inpayload.GetXML()
                print "\n ---"

                request_node = get_child_nodes(inpayload.Child())
                request = str(request_node[0].Get('name')) 
		
		
		print "Inside getInfo function of ExternalStorageInformationService .."
		information = self.externalInfo.getInfo(request)
		return create_response('externalStorageInformation:getInfo',
                               ['externalStorageInformation:hostname','externalStorageInformation:protocol','externalStorageInformation:port'], information, self.newSOAPPayload())
