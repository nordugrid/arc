"""
resource to query urls for
"""

# json module is stock in Python 2.6, for Python 2.5 we use simplejson
try:
    import json
except ImportError:
    import simplejson as json

from twisted.python import log
from twisted.web import resource



class IndexResource(resource.Resource):

    isLeaf = True

    def __init__(self, index):
        resource.Resource.__init__(self)
        self.index = index


    def render_GET(self, request):
        log.msg("Index get. Args:" + str(request.args))

        try:
            urls = request.args[b'url'][0].decode().split(',')
        except KeyError as e:
            log.msg("Couldn't get url argument from request")
            request.setResponseCode(400)
            return "Couldn't get url argument from request"

        log.msg("Query for urls: " + str(urls))
        result = self.index.query(urls)

        rv = json.dumps(result)

        request.setHeader(b'Content-type', b'application/json')
        request.setHeader(b'Content-length', str(len(rv)).encode())

        return rv.encode()

