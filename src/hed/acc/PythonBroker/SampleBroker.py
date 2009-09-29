'''
Example implementation of a custom broker written in python.
Invoke the broker with:

arcsub -b PythonBroker:SampleBroker.MyBroker:brokerargs
'''

import arc
import random

class MyBroker:

    def __init__(self, usercfg):

        # Extract some useful information from the broker configuration

        self.proxypath = usercfg.ProxyPath()
        self.certificatepath = usercfg.CertificatePath()
        self.keypath = usercfg.KeyPath()
        self.cacertificatesdir = usercfg.CACertificatesDirectory()
        pos = usercfg.Broker().second.find(':')
        if pos > 0:
            self.args = usercfg.Broker.second[pos + 1:]
        else:
            self.args = ""

    def SortTargets(self, PossibleTargets, job):

        # Some printouts - only as an example

        print 'Proxy Path:', self.proxypath
        print 'Certificate Path:', self.certificatepath
        print 'Key Path:', self.keypath
        print 'CA Certificates Dir:', self.cacertificatesdir

        print 'Broker arguments:', self.args

        print 'JobName:', job.Identification.JobName
        print 'Executable:', job.Application.Executable.Name
        for i in range(job.Application.Executable.Argument.size()):
            print 'Argument', i, ':', job.Application.Executable.Argument[i]

        # Broker implementation starts here

        print 'Targets before brokering:'

        for t in PossibleTargets:
            print t.url.str()

        print 'Randomizing...'

        random.shuffle(PossibleTargets)

        print 'Targets after brokering:'

        for t in PossibleTargets:
            print t.url.str()
