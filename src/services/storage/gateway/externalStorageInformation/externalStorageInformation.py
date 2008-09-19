"""
Gateway Component of ARC1 storage System. 

Service Class = ExternalStorageInformationService
Worker Class  = ExternalStorageInformation

Author: Salman Zubair Toor
email: salman.toor@it.uu.se

"""



import arc
from storage.common import externalInfo_uri, create_response
from storage.common import create_response, get_child_nodes, true
from storage.service import Service
import os 
class ExternalStorageInformation:

	def __init__(self):
	
		print "ExternalStorageInformation Constructor..." 
		
	def getInfo(self, request):
		
		""" This method receives the type of the external store to be used and
                 get the information from the configuration file externalStore.
                
                This method returns following information:
                hostname: hostname of the required type of the external store
                protocol: available protocol for that host
                port: for example 2811 for gridftp and 8443 for srm"""
		
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
							information[hostprotocolport[1]]= [hostprotocolport[0],hostprotocolport[2].rstrip('\n')]
						elif len(hostprotocolport) == 2:
							information[hostprotocolport[1].rstrip('\n')]= [hostprotocolport[0].rstrip('\n'),'2811']
						else:
							information["NoHostFound"]=['','']

	
		print information
		if information == {}:
			information["NoHostFound"]=['','']
		return information					
class ExternalStorageInformationService(Service):

	def __init__(self, cfg):
	
		print "ExternalStorageInformationService Constructor..."
		request_names = ['getInfo']
		Service.__init__(self,'ExternalStorageInformation', request_names, 'externalstorageinformation',externalInfo_uri)
		self.externalInfo = ExternalStorageInformation()
		
	def getInfo(self, inpayload):
		
		""" This method receives the type of the external store to be used and 
		pass this info to the working class of ExternalStorageInformationService
		
		This method returns following information:
		hostname: hostname of the required type of the external store
		protocol: available protocol for that host
		port: for example 2811 for gridftp and 8443 for srm"""


		print "\n --- \n Inside list function of ExternalStorageInformationService"
                print "Message from the client:", inpayload.GetXML()
                print "\n ---"

                request_node = get_child_nodes(inpayload.Child())
                request = str(request_node[0].Get('name')) 
		
		
		print "Inside getInfo function of ExternalStorageInformationService .."
		information = self.externalInfo.getInfo(request)
		return create_response('externalStorageInformation:getInfo',
                               ['externalStorageInformation:hostname','externalStorageInformation:protocol','externalStorageInformation:port'], information, self.newSOAPPayload())
