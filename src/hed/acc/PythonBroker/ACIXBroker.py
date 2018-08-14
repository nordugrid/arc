'''
Broker using ACIX information. This broker queries ACIX for cached locations
of input files specified in the job. It then matches those locations against
execution targets and ranks the targets by the number of cached files they
have.

Implements the following methods from BrokerPlugin:
 - bool operator() (const ExecutionTarget&, const ExecutionTarget&) const;
   - Used for sorting targets, here the method is lessthan()
 - bool match(const ExecutionTarget& et) const;
   - Returns whether the target matches
 - void set(const JobDescription& _j) const;
   - Set the job description which is to be brokered

Invoke the broker with:

arcsub -b PythonBroker:ACIXBroker.ACIXBroker:CacheIndexURL

e.g.
arcsub -b PythonBroker:ACIXBroker.ACIXBroker:https://cacheindex.ndgf.org:6443/data/index

or by setting in client.conf:
[common]
brokername=PythonBroker
brokerarguments=ACIXBroker.ACIXBroker:https://cacheindex.ndgf.org:6443/data/index

The PYTHONPATH must contain the path to this file, with a default installation
this is /usr/share/arc/examples/PythonBroker

The log level of this broker can only be set in client.conf, because that is
the only way that verbosity is set in the UserConfig object, eg
verbosity=DEBUG

The default level is WARNING.
'''

import random
try:
    import http.client as httplib
except ImportError:
    import httplib
import re
import logging

# Check if we can use json module (python >= 2.6 only)
try:
    import json
except ImportError:
    json = False

import arc

class ACIXBroker(object):
    def __init__(self, usercfg):
        '''
        Set up internal fields and get information from UserConfig.
        '''
        self.inputfiles = [] # list of remote input files to check
        self.cachelocations = {} # dict of files to cache locations (hostnames)
        self.targetranking = {} # dict of hostname to ranking (no of cached files)
        self.cacheindex = '' # URL of ACIX index service
        self.uc = usercfg # UserConfig object
        
        loglevel = 'WARNING'
        if usercfg.Verbosity():
            loglevel = usercfg.Verbosity().upper()
            # Map ARC to python log levels
            if loglevel == 'FATAL':
                loglevel = 'CRITICAL'
            if loglevel == 'VERBOSE':
                loglevel = 'INFO' 
            
        logging.basicConfig(format='%(levelname)s: ACIXBroker: %(message)s', level=getattr(logging, loglevel.upper()))
        
        brokerarg = usercfg.Broker()[1]
        if brokerarg.find(':') != -1:
            self.cacheindex = brokerarg[brokerarg.find(':') + 1:]
        logging.info('cache index: %s', self.cacheindex)
        
        # Check ACIX URL is valid
        (procotol, host, port, path) = self.splitURL(self.cacheindex)
        if not host or not path:
            logging.error('Invalid URL for ACIX index: %s', self.cacheindex)
            self.cacheindex = ''

    def match(self, target):
        '''
        Check the number of cache files at target. All targets which match the
        job description are acceptable even if no files are cached. We assume
        only one A-REX per hostname, so multiple interfaces at the same
        hostname are ignored.
        '''
        (procotol, host, port, path) = self.splitURL(target.ComputingEndpoint.URLString)
        
        if not host or host in self.targetranking:
            return True
        
        # First do generic matching
        if not arc.Broker.genericMatch(target, self.job, self.uc):
            return False
        
        cached = 0
        for file in self.cachelocations:
            if host in self.cachelocations[file]:
                cached += 1
                
        self.targetranking[host] = cached
        logging.debug('host: %s, cached files: %i', host, cached)
            
        # Accept target in all cases even if no files are cached
        return True

    def set(self, jobdescription):
        '''
        Extract the input files from the job description and call ACIX to find
        cached locations.
        '''
        self.job = jobdescription
        
        if not self.job or not self.cacheindex:
            return

        self.getInputFiles(self.job.DataStaging.InputFiles)
        
        if not self.inputfiles:
            return
        
        self.queryACIX(0)

    def lessthan(self, lhs, rhs):
        '''
        Used to sort targets
        '''
        (lprocotol, lhost, lport, lpath) = self.splitURL(lhs.ComputingEndpoint.URLString)
        (rprocotol, rhost, rport, rpath) = self.splitURL(rhs.ComputingEndpoint.URLString)
        
        if not lhost or not rhost or lhost not in self.targetranking or rhost not in self.targetranking:
            return random.randint(0, 1)
        
        return self.targetranking[lhost] > self.targetranking[rhost]

    # Internal methods
    
    def getInputFiles(self, inputfilelist):
        '''
        Extract input files and add to our list.
        '''
        for inputfile in inputfilelist:
            # remote input files only
            if inputfile.Sources and inputfile.Sources[0].Protocol() != 'file':
                # Some job desc languages allow multiple sources - for now choose the first
                url = inputfile.Sources[0]
                
                # Use the same string representation of URL as the cache code
                # does, including making sure LFC guids are used properly
                canonic_url = url.plainstr()
                if url.MetaDataOption("guid"):
                    canonic_url += ":guid=" + url.MetaDataOption("guid")
      
                logging.debug('input file: %s', canonic_url)
                self.inputfiles.append(canonic_url)

    
    def queryACIX(self, index):
        '''
        Call ACIX index to get cached locations of self.inputfiles[index:].
        It seems like ACIX has a limit of 64k character URLs, so if we exceed
        that then call recursively.
        '''
        maxACIXurl = 60000
        (procotol, host, port, path) = self.splitURL(self.cacheindex)
                
        # add URLs to path
        path += '?url=' + self.inputfiles[index]
        index += 1
        for file in self.inputfiles[index:]:
            path += ',' + file
            index += 1
            if len(path) > maxACIXurl and index != len(self.inputfiles):
                logging.debug('URL length (%i) for ACIX query exceeds maximum (%i), will call in batches', len(path), maxACIXurl)
                self.queryACIX(index)
                break
            
        conn = httplib.HTTPSConnection(host, port)
        try:
            conn.request('GET', path)
        except Exception as e:
            logging.error('Error connecting to service at %s: %s', host, str(e))
            return
        
        try:
            resp = conn.getresponse()
        except httplib.HTTPException as e:
            logging.error('Bad response from ACIX: %s', str(e))
            return
        
        logging.info('ACIX returned %s %s', resp.status, resp.reason)
        
        data = resp.read()
        conn.close()
        logging.debug('ACIX response: %s', data)
        
        if json:
            try:
                self.cachelocations.update(json.loads(data))
            except ValueError:
                logging.error('Unexpected response from ACIX: %s', data)
        else:
            # Using eval() is unsafe but it will only be used on old OSes
            # (like RHEL5). At least check response looks like a python dictionary
            if data[0] != '{' or data[-1] != '}':
                logging.error('Unexpected response from ACIX: %s', data)
            else:
                self.cachelocations.update(eval(data))

    def splitURL(self, url):
        """
        Split url into (protocol, host, port, path) and return this tuple.
        """
        match = re.match('(\w*)://([^/?#:]*):?(\d*)/?(.*)', url)
        if match is None:
            logging.warning('URL %s is malformed', url)
            return ('', '', 0, '')
        
        port_s = match.group(3)
        if port_s:
            port = int(port_s)
        else:
            port = None
            
        urltuple = (match.group(1), match.group(2), port, '/'+match.group(4))
        return urltuple
