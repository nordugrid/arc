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

        self.proxypath = usercfg.ConfTree().Get('ProxyPath')
        self.certificatepath = usercfg.ConfTree().Get('CertificatePath')
        self.keypath = usercfg.ConfTree().Get('KeyPath')
        self.cacertificatesdir = usercfg.ConfTree().Get('CACertificatesDir')
        pos = str(usercfg.ConfTree().Get('Broker').Get('Arguments')).find(':')
        if pos > 0:
            self.args = str(usercfg.ConfTree().Get('Broker').Get('Arguments'))[pos + 1:]
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
