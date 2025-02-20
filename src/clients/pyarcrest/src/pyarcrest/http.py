import http.client
import json
import logging
import ssl
from http.client import HTTPConnection, HTTPSConnection, RemoteDisconnected
from urllib.parse import urlencode, urlparse

from pyarcrest.errors import HTTPClientError

log = logging.getLogger(__name__)


### Solution to output http.client logs to log instead of print

# https://stackoverflow.com/a/58769712
def print_to_log(*args):
    log.debug(" ".join(args))

http.client.print = print_to_log

###


# TODO: blocksize is not used until Python 3.7 becomes minimum version
class HTTPClient:

    def __init__(self, url=None, host=None, port=None, blocksize=None, timeout=None, proxypath=None, isHTTPS=True):
        """Process parameters and create HTTP connection."""
        if url:
            parts = urlparse(url)
            if parts.scheme == "https" or parts.scheme == "":
                useHTTPS = True
            elif parts.scheme == "http":
                useHTTPS = False
            else:
                raise HTTPClientError(f"URL scheme not http(s) but {parts.scheme}")
            host = parts.hostname
            if not host:
                raise HTTPClientError("No hostname in URL")
            port = parts.port

        else:
            if not host:
                raise HTTPClientError("No hostname parameter")
            useHTTPS = isHTTPS
            port = port

        if proxypath:
            if not useHTTPS:
                raise HTTPClientError("Cannot use proxy without HTTPS")
            else:
                context = ssl.SSLContext(ssl.PROTOCOL_TLS)
                context.load_cert_chain(proxypath)
        else:
            context = None

        kwargs = {}
        # TODO: must not pass for as long as python 3.6 is used
        #if blocksize is not None:
        #    kwargs["blocksize"] = blocksize
        if timeout:
            kwargs["timeout"] = timeout

        if useHTTPS:
            if not port:
                port = 443
            self.conn = HTTPSConnection(host, port=port, context=context, **kwargs)
        else:
            if not port:
                port = 80
            self.conn = HTTPConnection(host, port=port, **kwargs)
        self.conn.set_debuglevel(1)

        self.isHTTPS = useHTTPS

    def request(self, method, endpoint, headers={}, token=None, jsonData=None, data=None, params={}):
        """Send request and retry on ConnectionErrors."""
        if token:
            headers['Authorization'] = f'Bearer {token}'

        if jsonData:
            body = json.dumps(jsonData).encode()
            headers['Content-Type'] = 'application/json'
        else:
            body = data

        for key, value in params.items():
            if isinstance(value, list):
                params[key] = ','.join([str(val) for val in value])

        query = ''
        if params:
            query = urlencode(params)

        if query:
            url = f'{endpoint}?{query}'
        else:
            url = endpoint

        try:
            self.conn.request(method, url, body=body, headers=headers)
            resp = self.conn.getresponse()
        # TODO: should the request be retried for aborted connection by peer?
        except (RemoteDisconnected, BrokenPipeError, ConnectionAbortedError, ConnectionResetError):
            # retry request
            try:
                self.conn.request(method, url, body=body, headers=headers)
                resp = self.conn.getresponse()
            except:
                self.close()
                raise
        except:
            self.close()
            raise

        return resp

    def close(self):
        """Close connection."""
        self.conn.close()
