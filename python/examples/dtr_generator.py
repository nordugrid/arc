#!/usr/bin/python2

# The nordugrid-arc-python package is required. As stated in the comments, the
# main missing piece in Python when compared to C++ is the ability to get
# callbacks to the Generator when the DTR has finished. To run:
#
# python dtr_generator.py /bin/ls /tmp/dtrtest
#
# If nordugrid-arc-python is installed to a non-standard location, PYTHONPATH
# may need to be set.

import os
import sys
import time
import arc

class DTRGenerator(arc.DTRCallback):

    def __init__(self):
        super(DTRGenerator, self).__init__()
        # Set up logging
        self.root_logger = arc.Logger_getRootLogger()
        self.stream = arc.LogStream(sys.stdout)
        self.root_logger.addDestination(self.stream)
        self.root_logger.setThreshold(arc.DEBUG)
        self.cfg = arc.UserConfig('', '')
        self.id = '1'
        arc.DTR.LOG_LEVEL = self.root_logger.getThreshold()

        # Start the Scheduler
        self.scheduler = arc.Scheduler()
        self.scheduler.start()

    def __del__(self):
        # Stop Scheduler when Generator is finished
        self.scheduler.stop()

    def add(self, source, dest):
        # Logger object, wrapped in smart pointer. The Logger object can only be accessed
        # by explicitly deferencing the smart pointer.
        dtrlog = arc.createDTRLogger(self.root_logger, "DTR")
        dtrlog.__deref__().addDestination(self.stream)
        dtrlog.__deref__().setThreshold(arc.DEBUG)

        # Create DTR (also wrapped in smart pointer)
        dtrptr = arc.createDTRPtr(source, dest, self.cfg, self.id, os.getuid(), dtrlog)

        # The ability to register 'this' as a callback object is not available yet
        #dtrptr.registerCallback(self, arc.GENERATOR)
        # Register the scheduler callback so we can push the DTR to it
        dtrptr.registerCallback(self.scheduler, arc.SCHEDULER)
        # Send the DTR to the Scheduler
        arc.DTR.push(dtrptr, arc.SCHEDULER)
        # Since the callback is not available, wait until the transfer reaches a final state
        while dtrptr.get_status() != arc.DTRStatus.ERROR and dtrptr.get_status() != arc.DTRStatus.DONE:
            time.sleep(1)
        sys.stdout.write("%s\n"%dtrptr.get_status().str())

    # This is never called in the current version
    def receiveDTR(self, dtr):
        sys.stdout.write('Received back DTR %s\n'%str(dtr.get_id()))

def main(args):
    if len(args) != 3:
        sys.stdout.write("Usage: python dtr_generator.py source destination\n")
        sys.exit(1)
    generator = DTRGenerator()
    generator.add(args[1], args[2])

if __name__ == '__main__':
    main(sys.argv[0:])
