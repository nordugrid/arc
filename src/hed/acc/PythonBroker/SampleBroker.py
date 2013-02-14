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
        pos = usercfg.Broker()[1].find(':')
        if pos > 0:
            self.args = usercfg.Broker()[1][pos + 1:]
        else:
            self.args = ""

    def set(self, job):
        # Either set the job description as a member object, or extract
        # the relevant information. Only printing below for clarity.
        print 'JobName:', job.Identification.JobName
        print 'Executable:', job.Application.Executable.Path
        for i in range(job.Application.Executable.Argument.size()):
            print 'Argument', i, ':', job.Application.Executable.Argument[i]

    def match(self, target):
        # Some printouts - only as an example
        print 'Proxy Path:', self.proxypath
        print 'Certificate Path:', self.certificatepath
        print 'Key Path:', self.keypath
        print 'CA Certificates Dir:', self.cacertificatesdir

        print 'Broker arguments:', self.args

        # Broker implementation starts here

        print 'Targets before brokering:'

        print target.ComputingEndpoint.URLString
        
        # Accept target
        return True

    def lessthan(self, lhs, rhs):
        print 'Randomizing...'
        return random.randint(0, 1)
