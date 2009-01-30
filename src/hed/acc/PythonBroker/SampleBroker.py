'''
Example implementation of a custom broker written in python.
Invoke the broker with:

arcsub -b PythonBroker:SampleBroker.MyBroker:brokerargs
'''

import arc
import random

class MyBroker:

    def __init__(self, cfg):

        # Extract some useful information from the broker configuration

        self.proxypath = cfg.Get('ProxyPath')
        self.certificatepath = cfg.Get('CertificatePath')
        self.keypath = cfg.Get('KeyPath')
        self.cacertificatesdir = cfg.Get('CACertificatesDir')
        pos = str(cfg.Get('Arguments')).find(':')
        if pos > 0:
            self.args = str(cfg.Get('Arguments'))[pos + 1:]
        else:
            self.args = ""

    def SortTargets(self, PossibleTargets, job):

        # Some printouts - only as an example

        print 'Proxy Path:', self.proxypath
        print 'Certificate Path:', self.certificatepath
        print 'Key Path:', self.keypath
        print 'CA Certificates Dir:', self.cacertificatesdir

        print 'Broker arguments:', self.args

        print 'JobName:', job.JobName
        print 'Executable:', job.Executable
        for i in range(job.Argument.size()):
            print 'Argument', i, ':', job.Argument[i]

        # Broker implementation starts here

        print 'Targets before brokering:'

        for t in PossibleTargets:
            print t.url.str()

        print 'Randomizing...'

        random.shuffle(PossibleTargets)

        print 'Targets after brokering:'

        for t in PossibleTargets:
            print t.url.str()
