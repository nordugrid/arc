import arc
from storage.service import Service
from storage.common import transfer_uri, create_response

class Transfer:

        def __init__(self):

                print "Transfer Constructor..."

        def transferData(self, requests):

                print "Inside transferData function of Transfer .."
		response = {}
		response['11'] = ('finished', 'successful')
		return response 

class TransferService(Service):

        def __init__(self, cfg):

                print "TransferService Constructor..."
                request_names = ['transferData']
                Service.__init__(self,'Transfer', request_names, 'transfer',transfer_uri)
                self.transfer = Transfer()

        def transferData(self, inpayload):

                print "Inside transferFile function of TransferService  .."
                requests = {'transferFile_request1':'first', 'transferFile_request2':'second'}
                response = self.transfer.transferData(requests)
		return create_response('transfer:transferData',
                               ['transfer:ID','transfer:status','transfer:trnasfer'], response, self.newSOAPPayload())
		
