import arc
from storage.common import externalInfo_uri, create_response
from storage.service import Service
class ExternalStorageInformation:

	def __init__(self):
	
		print "ExternalStorageInformation Constructor..." 
		
	def getInfo(self, requests):
		
		print "Inside getInfo function of ExternalStorageInformation .."	
				
class ExternalStorageInformationService(Service):

	def __init__(self, cfg):
	
		print "ExternalStorageInformationService Constructor..."
		request_names = ['getInfo']
		Service.__init__(self,'ExternalStorageInformation', request_names, 'externalstorageinformation',externalInfo_uri)
		self.externalInfo = ExternalStorageInformation()
		
	def getInfo(self, inpayload):
		response = {}	
		response['33'] = ('getInfo','')
		print "Inside getInfo function of ExternalStorageInformationService .."
		requests = {'getInfo_request1':'first', 'getInfo_request2':'second'}
		self.externalInfo.getInfo(requests)
		return create_response('externalStorageInformation:getInfo',
                               ['externalStorageInformation:ID','externalStorageInformation:say','externalStorageInformation:oo'], response, self.newSOAPPayload())
		#return create_response('externalStorageInformation:getInfo',
		#		['externalStorageInformation:ID','externalStorageInformation:say','externalStorageInformation:oo'], response, self.newSOAPPayload())
