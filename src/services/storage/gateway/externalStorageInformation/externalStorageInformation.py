import arc
from storage.common import externalInfo_uri, create_response
from storage.common import create_response, get_child_nodes, true
from storage.service import Service
import os 
class ExternalStorageInformation:

	def __init__(self):
	
		print "ExternalStorageInformation Constructor..." 
		
	def getInfo(self, request):
		
		filetext = []
		hostname = []
		print "Inside getInfo function of ExternalStorageInformation .."	
		#print os.getcwd()
		if request == 'dCache': 
			if (os.path.isfile("../externalStorageInformation/externaldCacheStore")):	
				f = open('../externalStorageInformation/externaldCacheStore', "r")
				for line in f.readlines():
					filetext.append(line)
					#print filetext
				for  host in filetext:
					hostname.append(host.split('=')[1].rstrip('\n'))
		else:
			hostname.append('NoHostFound')
		#text = "sal1.uppmax.uu.se"
		#print hostname
		return hostname					
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
		
		#print request
		
		response = {}	
		
		print "Inside getInfo function of ExternalStorageInformationService .."
		#requests = {'getInfo_request1':'first', 'getInfo_request2':'second'}
		hostname = self.externalInfo.getInfo(request)
		#print request
		if hostname != []:
			for i in range(len(hostname)):
				response[i] = (hostname[i],'OK')
		else:
			response[0] = ('','')	
		return create_response('externalStorageInformation:getInfo',
                               ['externalStorageInformation:ID','externalStorageInformation:hostname','externalStorageInformation:status'], response, self.newSOAPPayload())
		#return create_response('externalStorageInformation:getInfo',
		#		['externalStorageInformation:ID','externalStorageInformation:say','externalStorageInformation:oo'], response, self.newSOAPPayload())
