"""
Client for retrieving cache. Note that json is only available on python >= 2.6.
"""

import json
try:
    from urllib.parse import quote
except ImportError:
    from urllib import quote

from twisted.python import log
from twisted.web import client



class InvalidIndexReplyError(Exception):
    pass



def queryIndex(index_url, urls):
    for url in urls: assert ',' not in urls, "Commas ',' not allowed in urls currently"

    eurls = [ quote(url) for url in urls ]

    url = index_url + "?url=" + ','.join(eurls)

    d = client.getPage(url.encode())
    d.addCallback(_gotResult, index_url)
    d.addErrback(_indexError, index_url)
    return d


def _gotResult(result, index_url):
    log.msg("Got reply from index service %s" % index_url)

    try:
        decoded_result = json.loads(result)
        return decoded_result
    except ValueError as e:
        raise InvalidIndexReplyError(str(e))


def _indexError(failure, index_url):
    log.msg("Error while getting index results:")
    log.err(failure)
    return failure

